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

    // Step 1. Passes input through the model && updates logits, vels && offsets
    void forward_pass_v1(torch::Tensor monotonicGrooveInput);
    void forward_pass_v2_v3(torch::Tensor monotonicGrooveInput, torch::Tensor density);

    // Step 2. Sample hvo after the forward pass
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> sample(
        std::string sample_mode = "SampleProbability");

    // store path locally
    std::string model_path;

    // utils
    bool is_version1_vae();
    bool is_version2_vae();
    bool is_version3_vae();
    void randomize_latent_z();

private:

    torch::jit::script::Module model;
    int time_steps;
    int num_voices;
    torch::Tensor latent_z;
    torch::Tensor hits_logits;
    torch::Tensor hits_probabilities;
    torch::Tensor hits;
    torch::Tensor velocities;
    torch::Tensor offsets;
    torch::Tensor per_voice_sampling_thresholds;            // per voice thresholds for sampling
    torch::Tensor per_voice_max_count_allowed;              // per voice Maximum limit of hits
    float sampling_temperature {1.0f};
    int vae_type {-1};
    int latent_z_dim {-1};
    torch::jit::script::Module LoadModel(std::string model_path);
};

