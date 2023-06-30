//
// Created by Behzad Haki on 2022-08-03.
//

#include "VAE_V1ModelAPI.h"
#include <shared_plugin_helpers/shared_plugin_helpers.h>  // fixme remove this


using namespace std;

/*template<typename Iterator>
inline std::vector<size_t> n_largest_indices(Iterator it, Iterator end, size_t n) {
    struct Element {
        Iterator it;
        size_t index;
    };

    std::vector<Element> top_elements;
    top_elements.reserve(n + 1);

    for(size_t index = 0; it != end; ++index, ++it) {
        top_elements.insert(std::upper_bound(top_elements.begin(), top_elements.end(), *it, [](auto value, auto element){return value > *element.it;}), {it, index});
        if (index >= n)
            top_elements.pop_back();
    }

    std::vector<size_t> result;
    result.reserve(top_elements.size());

    for(auto &element: top_elements)
        result.push_back(element.index);

    return result;
}*/


static torch::Tensor vector2tensor(vector<float> vec)
{
    auto size = (int) vec.size();
    auto t = torch::zeros({size}, torch::kFloat32);
    for (size_t i=0; i<size; i++)
    {
        t[i] = vec[i];
    }

    return t;
}


//default constructor
VAE_V1ModelAPI::VAE_V1ModelAPI(){
    model_path = "NO Path Specified Yet!!";
}

// Loads model either in eval mode || train modee
inline torch::jit::script::Module VAE_V1ModelAPI::LoadModel(std::string model_path)
{
    torch::jit::script::Module model;
    model = torch::jit::load(model_path);
    model.eval();
    return model;
}

bool VAE_V1ModelAPI::loadModel(std::string model_path_, int time_steps_, int num_voices_)
{

    model_path = model_path_;
    time_steps = time_steps_;
    num_voices = num_voices_;
    ifstream myFile;
    myFile.open(model_path);
    // Check for errors

    // check if path works fine
    if (myFile.fail())
    {
        return false;
    }
    else
    {
        myFile.close();

        model = LoadModel(model_path);
        hits_logits = torch::zeros({time_steps_, num_voices_});
        hits_probabilities = torch::zeros({time_steps_, num_voices_});
        hits = torch::zeros({time_steps_, num_voices_});
        velocities = torch::zeros({time_steps_, num_voices_});
        offsets = torch::zeros({time_steps_, num_voices_});
        per_voice_sampling_thresholds = vector2tensor(nine_voice_kit_default_sampling_thresholds);
        per_voice_max_count_allowed = vector2tensor(nine_voice_kit_default_max_voices_allowed);
        return true;
    }

}

bool VAE_V1ModelAPI::changeModel(std::string model_path_)
{
    ifstream myFile;
    myFile.open(model_path_.append("/InputLayerEncoder.pt"));
    // Check for errors

    // check if path works fine
    if (myFile.fail())
    {
        return false;
    }
    else
    {
        myFile.close();
        model = LoadModel(model_path);
        return true;
    }
}

// getters
torch::Tensor VAE_V1ModelAPI::get_hits_logits() { return hits_logits; }
torch::Tensor VAE_V1ModelAPI::get_hits_probabilities() { return hits_probabilities; }
torch::Tensor VAE_V1ModelAPI::get_velocities() { return velocities; }
torch::Tensor VAE_V1ModelAPI::get_offsets() { return offsets; }


// setters
void VAE_V1ModelAPI::set_sampling_thresholds(vector<float> per_voice_thresholds)
{
    assert(per_voice_thresholds.size()==num_voices &&
           "thresholds dim [num_voices]");

    per_voice_sampling_thresholds = vector2tensor(per_voice_thresholds);
}

void VAE_V1ModelAPI::set_max_count_per_voice_limits(vector<float> perVoiceMaxNumVoicesAllowed)
{
    assert(perVoiceMaxNumVoicesAllowed.size()==num_voices &&
           "thresholds dim [num_voices]");

    per_voice_max_count_allowed = vector2tensor(perVoiceMaxNumVoicesAllowed);
}

// returns true if temperature has changed
bool VAE_V1ModelAPI::set_sampling_temperature(float temperature)
{
    if (sampling_temperature == temperature)
    {
        return false;
    }
    sampling_temperature = temperature;
    DBG("TEMPERATURE UPDATED to " << sampling_temperature);
    return true;
}

// Passes input through the model && updates logits, vels && offsets
void VAE_V1ModelAPI::forward_pass(torch::Tensor monotonicGrooveInput)
{
    assert(monotonicGrooveInput.sizes()[0]==time_steps &&
           "shape [time_steps, num_voices*3]");
    assert(monotonicGrooveInput.sizes()[1]==(num_voices*3) &&
           "shape [time_steps, num_voices*3]");

    // flatten groove into single voice
    auto row_indices = torch::arange(0, time_steps);
    auto flat_groove = torch::zeros({time_steps, 3});
    flat_groove.index_put_({row_indices, torch::indexing::Slice(0, 1)}, monotonicGrooveInput.index({row_indices, torch::indexing::Slice(2, 3)}));
    flat_groove.index_put_({row_indices, torch::indexing::Slice(1, 2)}, monotonicGrooveInput.index({row_indices, torch::indexing::Slice(11, 12)}));
    flat_groove.index_put_({row_indices, torch::indexing::Slice(2, 3)}, monotonicGrooveInput.index({row_indices, torch::indexing::Slice(20, 21)}));

    // wrap as IValue vector && pass through InputLayerEncoder
    std::vector<torch::jit::IValue> inputs{flat_groove};
    auto encoder = model.get_method("encode");
    auto result = encoder(inputs);
    latent_z = result.toTuple()->elements()[2].toTensor();
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> VAE_V1ModelAPI::
    sample(std::string sample_mode)
{
    DBG(sample_mode);
    assert (sample_mode=="Threshold" || sample_mode=="SampleProbability");

    // set to 0 if sample_mode == "Threshold"
    int smp_md = 0;
    if (sample_mode=="SampleProbability")
    {
        smp_md = 1;
    }

    //https://stackoverflow.com/questions/69884828/how-to-input-an-int-argument-to-the-forward-function-of-torchjitscriptmodu
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(latent_z);
    inputs.push_back(per_voice_sampling_thresholds);
    inputs.push_back(per_voice_max_count_allowed);
    inputs.push_back(smp_md);  // Convert int to Tensor to IValue
    inputs.push_back(sampling_temperature);  // Convert float to Tensor to IValue
    DBG("sampling_temperature: " << sampling_temperature);


    auto sampler_method = model.get_method("sample");
    auto res = sampler_method(inputs);

    // hits is the first element of the tuple, remove the batch dim (first dim)
    hits = res.toTuple()->elements()[0].toTensor().squeeze(0);
    velocities = res.toTuple()->elements()[1].toTensor().squeeze(0);
    offsets = res.toTuple()->elements()[2].toTensor().squeeze(0);
    hits_probabilities = res.toTuple()->elements()[3].toTensor().squeeze(0);

    // convert hits tensor to a string for debugging
    //    std::stringstream ss;
    //    ss << hits;
    //    std::string hits_str = ss.str();
    //    DBG("hits: " << hits_str);
    //    auto sampler_method = model.get_method("sample");
    //
    //    auto res = sampler_method(
    //        hits_probabilities, velocities, offsets,
    //        per_voice_sampling_thresholds, per_voice_max_count_allowed,
    //        per_voice_max_count_allowed, sample_mode);

    return {hits, velocities, offsets};
}




