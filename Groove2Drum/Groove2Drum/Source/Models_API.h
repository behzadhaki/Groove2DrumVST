//
// Created by Behzad Haki on 2022-07-29.
//
#pragma once

#include <torch/script.h> // One-stop header.

// Loads model either in eval mode or train modee
inline torch::jit::script::Module LoadModel(std::string model_path, bool eval = true)
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

class MonotonicGrooveTransformerV1{
private:

    torch::jit::script::Module model;           // model to be loaded in constructor
    int time_steps;
    int num_voices;
    torch::Tensor hits_logits;
    torch::Tensor hits_probabilities;
    torch::Tensor hits;
    torch::Tensor velocities;
    torch::Tensor offsets;
    torch::Tensor sampling_thresholds;            // per voice thresholds for sampling

public:
    // constructor
    MonotonicGrooveTransformerV1(std::string model_path, int time_steps, int num_voices):
        time_steps(time_steps), num_voices(num_voices)
    {
        model = LoadModel(model_path, true);
        hits_logits = torch::zeros({time_steps, num_voices});
        hits_probabilities = torch::zeros({time_steps, num_voices});
        hits = torch::zeros({time_steps, num_voices});
        velocities = torch::zeros({time_steps, num_voices});
        offsets = torch::zeros({time_steps, num_voices});

        sampling_thresholds = default_sampling_thresholds;

    }

    // getters
    torch::Tensor get_hits_logits() { return hits_logits; }
    torch::Tensor get_hits_probabilities() { return hits_probabilities; }
    torch::Tensor get_velocities() { return velocities; }
    torch::Tensor get_offsets() { return offsets; }

    // setters
    torch::Tensor set_sampling_thresholds(torch::Tensor per_voice_thresholds)
    {
        assert(per_voice_thresholds.sizes()[0]==num_voices &&
               "thresholds dim [num_voices]");

        sampling_thresholds = per_voice_thresholds;
    }


    // Passes input through the model and updates logits, vels and offsets
    void forward_pass(torch::Tensor monotonicGrooveInput)
    {
        assert(monotonicGrooveInput.sizes()[0]==time_steps &&
               "shape [time_steps, num_voices*3]");
        assert(monotonicGrooveInput.sizes()[1]==(num_voices*3) &&
               "shape [time_steps, num_voices*3]");

        std::vector<torch::jit::IValue> inputs;
        inputs.emplace_back(monotonicGrooveInput);
        auto outputs = model.forward(inputs);
        auto hLogit_v_o_tuples = outputs.toTuple();
        hits_logits = hLogit_v_o_tuples->elements()[0].toTensor();
        hits_probabilities = torch::sigmoid(hits_logits);
        velocities = hLogit_v_o_tuples->elements()[1].toTensor();
        offsets = hLogit_v_o_tuples->elements()[2].toTensor();
    }

    /*void sample_hits(std::string sample_mode = "Threshold"):
    {
        assert (sample_mode=="Threshold" or sample_mode=="SampleProbability");

        hits = torch::zeros({time_steps, num_voices});

        if (sample_mode=="Threshold")
        {
            // read CPU accessors in  https://pytorch.org/cppdocs/notes/tensor_basics.html
            auto hits_probabilities_a = hits_probabilities.accessor<float,2>();

            for (int i=0; i < num_voices; i++){
                auto thres  = sampling_thresholds[i][0];
                //todo https://discuss.pytorch.org/t/libtorch-tensor-indexing/105868/4
                //hits_probabilities_a
            }
        }


    }*/




    //


    /*torch::Tensor sample_hits(std::string sample_mode, int thresholds)
    {

    }*/

};

