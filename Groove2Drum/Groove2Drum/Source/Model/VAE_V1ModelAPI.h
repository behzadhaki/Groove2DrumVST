//
// Created by Behzad Haki on 2022-08-03.
//

#pragma once

#include <torch/script.h> // One-stop header.
#include "../settings.h"

using namespace torch::indexing;
using namespace std;

/**
 * API for loading the trained MonotonicGroove model
 */
class VAE_V1ModelAPI
{
public:
    // constructor
    VAE_V1ModelAPI();

    // loads a model
    bool loadModel(std::string model_path, int time_steps_, int num_voices_);

    // change a model
    bool changeModel(std::string model_path);

    // getters
    torch::Tensor get_hits_logits();
    torch::Tensor get_hits_probabilities();
    torch::Tensor get_velocities();
    torch::Tensor get_offsets();

    // setters
    void set_sampling_thresholds(vector<float> per_voice_thresholds);
    void set_max_count_per_voice_limits(vector<float> perVoiceMaxNumVoicesAllowed);
    bool set_sampling_temperature(float temperature);

    // Step 1. Passes input through the model and updates logits, vels and offsets
    void forward_pass(torch::Tensor monotonicGrooveInput, float scale_log_var);
    // Step 2. Sample hvo after the forward pass
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> sample(
        std::string sample_mode = "SampleProbability");

    // store path locally
    std::string model_path;

private:

    torch::jit::script::Module InputLayerEncoder;
    torch::jit::script::Module Encoder;
    torch::jit::script::Module LatentEncoder;
    torch::jit::script::Module Decoder;

    int time_steps{32};
    int num_voices{9};
    torch::Tensor hits_logits;
    torch::Tensor hits_probabilities;
    torch::Tensor hits;
    torch::Tensor velocities;
    torch::Tensor offsets;
    torch::Tensor per_voice_sampling_thresholds;            // per voice thresholds for sampling
    vector<float> per_voice_max_count_allowed;              // per voice Maximum limit of hits
    float sampling_temperature {1.0f};

    torch::jit::script::Module LoadModel(const std::string& model_path);
};

