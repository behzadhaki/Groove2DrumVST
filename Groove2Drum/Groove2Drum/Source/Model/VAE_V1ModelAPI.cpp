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

// Loads model either in eval mode or train modee
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
        DBG("Model Path is OK");
        InputLayerEncoder = LoadModel(model_path + "/InputLayerEncoder.pt");
        DBG("InputLayerEncoder is OK");
        DBG(model_path);
        Encoder = LoadModel(model_path + "/Encoder.pt");
        DBG("Encoder is OK");
        LatentEncoder = LoadModel(model_path + "/LatentEncoder.pt");
        DBG("LatentEncoder is OK");
        Decoder = LoadModel(model_path + "/Decoder.pt");
        DBG("Decoder is OK");

        hits_logits = torch::zeros({time_steps_, num_voices_});
        hits_probabilities = torch::zeros({time_steps_, num_voices_});
        hits = torch::zeros({time_steps_, num_voices_});
        velocities = torch::zeros({time_steps_, num_voices_});
        offsets = torch::zeros({time_steps_, num_voices_});
        per_voice_sampling_thresholds = vector2tensor(nine_voice_kit_default_sampling_thresholds);
        per_voice_max_count_allowed = nine_voice_kit_default_max_voices_allowed;
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
        model_path = model_path_;
        InputLayerEncoder = LoadModel(model_path.append("/InputLayerEncoder.pt"));
        Encoder = LoadModel(model_path.append("/Encoder.pt"));
        LatentEncoder = LoadModel(model_path.append("/LatentEncoder.pt"));
        Decoder = LoadModel(model_path.append("/Decoder.pt"));
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

    per_voice_max_count_allowed = perVoiceMaxNumVoicesAllowed;
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

// Passes input through the model and updates logits, vels and offsets
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

    // wrap as IValue vector and pass through InputLayerEncoder
    std::vector<torch::jit::IValue> inputs{flat_groove};
    //inputs.emplace_back(flat_groove);
    auto embedded = InputLayerEncoder.forward(inputs);

    // pass through Encoder'
    inputs.clear();
    inputs.emplace_back(embedded);
    auto encoded = Encoder.forward(inputs);

    // pass through LatentEncoder
    inputs.clear();
    inputs.emplace_back(encoded);
    auto latent = LatentEncoder.forward(inputs);

    // latent is a tuple of 3 tensors (mean, logvar, z)
    auto latent_tuple = latent.toTuple();
    auto mean = latent_tuple->elements()[0].toTensor();
    auto logvar = latent_tuple->elements()[1].toTensor();
    auto z = latent_tuple->elements()[2].toTensor();

    // pass through Decoder
    inputs.clear();
    inputs.emplace_back(z);
    auto outputs = Decoder.forward(inputs);

    auto hLogit_v_o_tuples = outputs.toTuple();
    hits_logits = hLogit_v_o_tuples->elements()[0].toTensor().view({time_steps, num_voices});
    hits_probabilities = torch::sigmoid(hits_logits/sampling_temperature).view({time_steps, num_voices});
    velocities = torch::sigmoid(hLogit_v_o_tuples->elements()[1].toTensor().view({time_steps, num_voices}));
    offsets = torch::sigmoid(hLogit_v_o_tuples->elements()[2].toTensor().view({time_steps, num_voices})) - 0.5f;
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> VAE_V1ModelAPI::
    sample(std::string sample_mode)
{
    DBG(sample_mode);
    assert (sample_mode=="Threshold" or sample_mode=="SampleProbability");

    hits = torch::zeros({time_steps, num_voices});

    auto row_indices = torch::arange(0, time_steps);
    if (sample_mode=="Threshold")
    {
        // read CPU accessors in  https://pytorch.org/cppdocs/notes/tensor_basics.html
        // asserts accessed part of tensor is 2-dimensional and holds floats.
        //auto hits_probabilities_a = hits_probabilities.accessor<float,2>();

        for (int voice_i=0; voice_i < num_voices; voice_i++){
            // Get voice threshold value
            auto thres_voice_i  = per_voice_sampling_thresholds[voice_i];
            // Get probabilities of voice hits at all timesteps
            auto voice_hit_probs = hits_probabilities.index(
                {row_indices, voice_i});
            auto tup = voice_hit_probs.topk((int) per_voice_max_count_allowed[size_t(voice_i)]);
            auto candidate_probs = std::get<0>(tup);
            auto candidate_prob_indices = std::get<1>(tup);

            // Find locations exceeding threshold and set to 1 (hit)
            auto accepted_candidate_indices = candidate_probs>=thres_voice_i;
            auto active_time_indices = candidate_prob_indices.index({accepted_candidate_indices});

            hits.index_put_({active_time_indices, voice_i}, 1);
        }
    } else if (sample_mode=="SampleProbability")
    {
        for (int voice_i=0; voice_i < num_voices; voice_i++){
            // Get voice threshold value
            auto thres_voice_i  = per_voice_sampling_thresholds[voice_i];
            // Get probabilities of voice hits at all timesteps
            auto voice_hit_probs = hits_probabilities.index(
                {row_indices, voice_i});
            voice_hit_probs = torch::where(voice_hit_probs>=thres_voice_i, voice_hit_probs, 0);

            hits.index_put_({row_indices, voice_i}, torch::bernoulli(voice_hit_probs));
        }
    }

    // Set non-hit vel and offset values to 0
    // velocities = velocities * hits;
    // offsets = offsets * hits;

    // DBG(tensor2string(hits));

    return {hits, velocities, offsets};
}




