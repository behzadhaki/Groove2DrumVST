#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Includes/UtilityMethods.h"

using namespace std;



MidiFXProcessor::MidiFXProcessor():
    apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
    DBG("MidiFXProcessor Constructor");

    std::vector<torch::jit::IValue> inputs{};
    inputs.emplace_back(torch::ones({100, 1}));
    inputs.emplace_back((int) 2);  // Convert int to Tensor to IValue


    model_paths = get_monotonic_v1_pt_files_in_default_path();
    model_paths.addArray(get_vae_files_in_default_path());
    instrument_model_paths = get_groove_converter_files_in_default_path();

    //////////////////////////////////////////////////////////////////
    //// Make_unique pointers for Queues
    //////////////////////////////////////////////////////////////////
    // GuiIOFifos
    GrooveThread2GGroovePianoRollWidgetQue = make_unique<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>>();
    ModelThread2GroovePianoRollWidgetQue = make_unique<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::gui_io_queue_size>>();
    DBG("HERE");
    // Intra Processor Threads
    ProcessBlockToGrooveThreadQue = make_shared<LockFreeQueue<BasicNote, GeneralSettings::processor_io_queue_size>>();
    GrooveThreadToModelThreadQue = make_shared<MonotonicGrooveQueue<HVO_params::time_steps, GeneralSettings::processor_io_queue_size>>();
    ModelThreadToProcessBlockQue = make_shared<GeneratedDataQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::processor_io_queue_size>>();
    APVTS2GrooveThread_groove_vel_offset_ranges_Que = make_shared<LockFreeQueue<std::array<float, 6>, GeneralSettings::gui_io_queue_size>>();
    APVTS2GrooveThread_groove_record_overdubToggles_Que = make_shared<LockFreeQueue<std::array<int, 2>, GeneralSettings::gui_io_queue_size>>();
    APVTS2ModelThread_max_num_hits_Que = make_shared<LockFreeQueue<std::array<float, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>>();
    APVTS2ModelThread_sampling_thresholds_and_temperature_and_density_Que = make_shared<LockFreeQueue<std::array<float, HVO_params::num_voices+2>, GeneralSettings::gui_io_queue_size>>();
    GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue = make_shared<LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>>();
    APVTS2ModelThread_midi_mappings_Que = make_shared<LockFreeQueue<std::array<int, HVO_params::num_voices>, GeneralSettings::gui_io_queue_size>>();
    ModelThreadToDrumPianoRollWidgetQue = make_unique<HVOLightQueue<HVO_params::time_steps, HVO_params::num_voices, GeneralSettings::gui_io_queue_size>>();

    //////////////////////////////////////////////////////////////////
    //// Create shared pointers for Threads (shared with APVTSMediator)
    //////////////////////////////////////////////////////////////////
    grooveThread = make_shared<GrooveThread>();
    modelThread = make_shared<ModelThread>();
    apvtsMediatorThread = make_unique<APVTSMediatorThread>(grooveThread.get(), modelThread.get());
        
    /////////////////////////////////
    //// Start Threads
    /////////////////////////////////

    // give access to resources && run threads
    modelThread->startThreadUsingProvidedResources(GrooveThreadToModelThreadQue.get(),
                                                   ModelThread2GroovePianoRollWidgetQue.get(),
                                                  ModelThreadToProcessBlockQue.get(),
                                                  ModelThreadToDrumPianoRollWidgetQue.get(),
                                                  APVTS2ModelThread_max_num_hits_Que.get(),
                                                   APVTS2ModelThread_sampling_thresholds_and_temperature_and_density_Que.get(),
                                                  APVTS2ModelThread_midi_mappings_Que.get());

    grooveThread->startThreadUsingProvidedResources(ProcessBlockToGrooveThreadQue.get(),
                                                   GrooveThreadToModelThreadQue.get(),
                                                   GrooveThread2GGroovePianoRollWidgetQue.get(),
                                                   GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue.get(),
                                                   APVTS2GrooveThread_groove_vel_offset_ranges_Que.get(),
                                                   APVTS2GrooveThread_groove_record_overdubToggles_Que.get());

    apvtsMediatorThread->startThreadUsingProvidedResources(&apvts,
                                                          APVTS2GrooveThread_groove_vel_offset_ranges_Que.get(),
                                                          APVTS2GrooveThread_groove_record_overdubToggles_Que.get(),
                                                          APVTS2ModelThread_max_num_hits_Que.get(),
        APVTS2ModelThread_sampling_thresholds_and_temperature_and_density_Que.get(),
                                                          APVTS2ModelThread_midi_mappings_Que.get());
}

MidiFXProcessor::~MidiFXProcessor(){
    if (!modelThread->readyToStop)
    {
        modelThread->prepareToStop();
    }

    if (!grooveThread->readyToStop)
    {
        grooveThread->prepareToStop();
    }

    if (!apvtsMediatorThread->readyToStop)
    {
        apvtsMediatorThread->prepareToStop();
    }
}

void MidiFXProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    tempBuffer.clear();

    // STEP 1
    // get Playhead info && buffer size && sample rate from host
    auto playhead = getPlayHead();
    auto Pinfo = playhead->getPosition();
    auto fs = getSampleRate();
    auto buffSize = buffer.getNumSamples();


    // STEP 2
    // check if new pattern is generated && available for playback
    if (ModelThreadToProcessBlockQue != nullptr)
    {
        if (ModelThreadToProcessBlockQue->getNumReady() > 0)
        {
            latestGeneratedData = ModelThreadToProcessBlockQue->getLatestOnly();
            DBG("new generated pattern received");
        }
    }

    // Step 3
    // In playback mode, add drum note to the buffer if the time is right
    if (Pinfo->getIsPlaying())
    {
        if (*Pinfo->getPpqPosition() < startPpq)
        {
            // if playback head moved backwards || playback paused && restarted
            // change the registration_times of groove events to ensure the
            // groove properly overdubs
            modelThread->scaled_groove.registeration_times.index({None, None}) = -100;
        }

        startPpq = *Pinfo->getPpqPosition();
        auto qpm = *Pinfo->getBpm();
        auto start_ = fmod(startPpq, HVO_params::time_steps/4); // start_ should be always between 0 && 8
        playhead_pos = fmod(float(startPpq + HVO_params::_32_note_ppq), float(HVO_params::time_steps/4.0f)) / (HVO_params::time_steps/4.0f);

        auto new_grid = floor(start_/HVO_params::_16_note_ppq);
        if (new_grid != current_grid)
        {
            // Ensures that the generations change every 2bars


            if (new_grid == 0)
            {
                auto param = apvts.getParameter("AUTO_REGENERATE");
                if (param->getValue() > 0){
                    grooveThread->RePushGroove();
                }
            }

            current_grid = new_grid;
            grooveThread->clearStep((int) current_grid, startPpq);
        }

        if (latestGeneratedData.numberOfGenerations() > 0)
        {
            for (size_t idx = 0; idx < (size_t) latestGeneratedData.numberOfGenerations(); idx++)
            {
                auto ppqs_from_start_ = latestGeneratedData.ppqs[idx] - start_;
                auto samples_from_start_ = ppqs_from_start_ * (60 * fs) / qpm;

                if (ppqs_from_start_>=0 && samples_from_start_<buffSize)
                {
                    DBG("PLAYING");
                    DBG(latestGeneratedData.midiMessages[idx].getFloatVelocity());
                    // send note on
                    tempBuffer.addEvent(latestGeneratedData.midiMessages[idx], (int) samples_from_start_);
                    // send note off
                    tempBuffer.addEvent(juce::MidiMessage::noteOff((int) 1, (int) latestGeneratedData.midiMessages[idx].getNoteNumber(), (float) 0), (int) samples_from_start_);
                }
            }
        }
    }

    // Step 4. see if new notes are played on the input side
    if (!midiMessages.isEmpty() /*and groove_thread_ready*/)
    {
        // send BasicNotes to the GrooveThread && also gui logger for notes
        place_BasicNote_in_queue<GeneralSettings::processor_io_queue_size>(midiMessages, Pinfo, ProcessBlockToGrooveThreadQue.get(), fs);
        // place_BasicNote_in_queue<GeneralSettings::gui_io_queue_size>(midiMessages, Pinfo, ProcessBlockToGrooveThreadQue, fs);
    }

    midiMessages.swapWith(tempBuffer);


    buffer.clear(); //
}

juce::AudioProcessorEditor* MidiFXProcessor::createEditor()
{
    auto editor = new MidiFXProcessorEditor(*this);
    /*modelThread.addChangeListener(editor);*/
    return editor;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidiFXProcessor();
}

float MidiFXProcessor::get_playhead_pos()
{
    return playhead_pos;
}


juce::AudioProcessorValueTreeState::ParameterLayout MidiFXProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    int version_hint = 1;
    // toggles
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("OVERDUB", version_hint), "OVERDUB", 0, 1, 1));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RECORD", version_hint), "RECORD", 0, 1, 1));

    // model selector combobox
    auto modelfiles = get_monotonic_v1_pt_files_in_default_path();
    modelfiles.addArray(get_vae_files_in_default_path());
    DBG("Found " + juce::String(modelfiles.size()) + " models");
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("MODEL", version_hint), "MODEL", 0, modelfiles.size()-1, 0));

    // groove_converter (i2g - instrument2drum) model selector combobox
    auto i2g_modelfiles= get_groove_converter_files_in_default_path();
    DBG("Found " + juce::String(modelfiles.size()) + " models");
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("I2G", version_hint), "I2G", 0, i2g_modelfiles.size()-1, 0));

    // buttons
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RESET_GROOVE", version_hint), "RESET_GROOVE", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RESET_SAMPLINGPARAMS", version_hint), "RESET_SAMPLINGPARAMS", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RESET_ALL", version_hint), "RESET_ALL", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOMIZE_VEL", version_hint), "RANDOMIZE_VEL", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOMIZE_OFFSET", version_hint), "RANDOMIZE_OFFSET", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOMIZE_ALL", version_hint), "RANDOMIZE_ALL", 0, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("RANDOM_GENERATION", version_hint), "RANDOM_GENERATION", 0, 1, 0));
    // xy velocity pad
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("VEL_DYNAMIC_RANGE", version_hint), "VEL_DYNAMIC_RANGE", 0.0f, 200.0f, 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("VEL_BIAS", version_hint), "VEL_BIAS", -0.60f, 1.0f, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("VEL_INVERT", version_hint), "VEL_INVERT", 0, 1, 0));

    // sliders
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("OFFSET_DYNAMIC_RANGE", version_hint), "OFFSET_DYNAMIC_RANGE", 0.0f, 200.0f, 100.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("OFFSET_BIAS", version_hint), "OFFSET_BIAS", -1, 1, 0));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("OFFSET_INVERT", version_hint), "OFFSET_INVERT", 0, 1, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("TEMPERATURE", version_hint), "TEMPERATURE", 0.00001f, 10.0f, 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("DENSITY", version_hint), "DENSITY", 0.00001f, 1.0f, 0.5f));

    // toggle for bernoulli sampling
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("SAMPLINGMETHOD", version_hint), "SAMPLINGMETHOD", 0, 1, 0));

    // slider for automated regeneration
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID("AUTO_REGENERATE", version_hint), "AUTO_REGENERATE", 0, 1, 0));

    // these parameters are used with the xySliders for each individual voice
    // Because xySliders are neither slider nor button, we
    for (size_t i=0; i < HVO_params::num_voices; i++)
    {
        auto voice_label = nine_voice_kit_labels[i];
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID(voice_label+"_X", version_hint), voice_label+"_X", 0.f, HVO_params::time_steps, nine_voice_kit_default_max_voices_allowed[i])); // max num voices
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID(voice_label+"_Y", version_hint), voice_label+"_Y", 0.f, 1.f, nine_voice_kit_default_sampling_thresholds[i]));                   // threshold level for sampling
    }

    // midi sliders
    for (size_t i=0; i < HVO_params::num_voices; i++)
    {
        auto voice_label = nine_voice_kit_labels[i];
        layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID(voice_label+"_MIDI", version_hint), voice_label+"_MIDI", 0, 127, (int) nine_voice_kit_default_midi_numbers[i])); // drum voice midi numbers
    }

    return layout;
}
LockFreeQueue<BasicNote, GeneralSettings::gui_io_queue_size>* MidiFXProcessor::
    get_pointer_GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue()
{
    return GroovePianoRollWidget2GrooveThread_manually_drawn_noteQue.get();
}

void MidiFXProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MidiFXProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

