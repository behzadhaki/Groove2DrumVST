//
// Created by Behzad Haki on 2022-08-03.
//

#include "ModelAPI.h"
#include "../settings.h"

//default constructor
MonotonicGrooveTransformerV1::MonotonicGrooveTransformerV1(){}

MonotonicGrooveTransformerV1::MonotonicGrooveTransformerV1(std::string model_path, int time_steps, int num_voices):
    time_steps(time_steps), num_voices(num_voices),per_voice_sampling_thresholds(default_sampling_thresholds)
{
    model = LoadModel(model_path, true);
    hits_logits = torch::zeros({time_steps, num_voices});
    hits_probabilities = torch::zeros({time_steps, num_voices});
    hits = torch::zeros({time_steps, num_voices});
    velocities = torch::zeros({time_steps, num_voices});
    offsets = torch::zeros({time_steps, num_voices});
}

// getters
torch::Tensor MonotonicGrooveTransformerV1::get_hits_logits() { return hits_logits; }
torch::Tensor MonotonicGrooveTransformerV1::get_hits_probabilities() { return hits_probabilities; }
torch::Tensor MonotonicGrooveTransformerV1::get_velocities() { return velocities; }
torch::Tensor MonotonicGrooveTransformerV1::get_offsets() { return offsets; }


// setters
void MonotonicGrooveTransformerV1::set_sampling_thresholds(torch::Tensor per_voice_thresholds)
{
    assert(per_voice_thresholds.sizes()[0]==num_voices &&
           "thresholds dim [num_voices]");

    per_voice_sampling_thresholds = per_voice_thresholds;
}


// Passes input through the model and updates logits, vels and offsets
void MonotonicGrooveTransformerV1::forward_pass(torch::Tensor monotonicGrooveInput)
{
    assert(monotonicGrooveInput.sizes()[0]==time_steps &&
           "shape [time_steps, num_voices*3]");
    assert(monotonicGrooveInput.sizes()[1]==(num_voices*3) &&
           "shape [time_steps, num_voices*3]");

    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(monotonicGrooveInput);
    auto outputs = model.forward(inputs);
    auto hLogit_v_o_tuples = outputs.toTuple();
    hits_logits = hLogit_v_o_tuples->elements()[0].toTensor().view({time_steps, num_voices});
    hits_probabilities = torch::sigmoid(hits_logits).view({time_steps, num_voices});
    velocities = hLogit_v_o_tuples->elements()[1].toTensor().view({time_steps, num_voices});
    offsets = hLogit_v_o_tuples->elements()[2].toTensor().view({time_steps, num_voices});
}

std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> MonotonicGrooveTransformerV1::
    sample(std::string sample_mode)
{
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
            auto voice_hot_probs = hits_probabilities.index(
                {row_indices, voice_i});
            // Find locations exceeding threshold and set to 1 (hit)
            auto active_time_indices = voice_hot_probs>=thres_voice_i;
            hits.index_put_({active_time_indices, voice_i}, 1);
        }
    }

    // Set non-hit vel and offset values to 0
    velocities = velocities * hits;
    offsets = offsets * hits;

    // DBG(tensor2string(hits));

    return {hits, velocities, offsets};
}

// ------------------------------------------------------------
// -------------------UTILS------------------------------------
// ------------------------------------------------------------
// converts a tensor to string to be used with DBG for debugging
std::string tensor2string (torch::Tensor tensor)
{
    std::ostringstream stream;
    stream << tensor;
    std::string tensor_string = stream.str();
    return tensor_string;
}

// Loads model either in eval mode or train modee
inline torch::jit::script::Module LoadModel(std::string model_path, bool eval)
{
    torch::jit::script::Module model;
    model = torch::jit::load(model_path);
    if (eval)
    {
        model.eval();
    }
    else
    {
        model.train();
    }
    return model;
}
