//
// Created by Behzad Haki on 2023-03-23.
//

#pragma once    

#include <torch/script.h> // One-stop header.
#include "../settings.h"

using namespace torch::indexing;
using namespace std;


class ModelsAPI
{
public:
    // constructor
    ModelsAPI();
    
    // loads a model
    bool loadModel(std::string model_path, int time_steps_, int num_voices_){
        model_path = model_path;
        time_steps = time_steps_;
        num_voices = num_voices_;
        MonotonicModelV1 = LoadModel(model_path);
        return true;
    }

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
    void forward_pass(torch::Tensor monotonicGrooveInput);
    // Step 2. Sample hvo after the forward pass
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> sample(
        std::string sample_mode = "SampleProbability");

    // store path locally
    std::string model_path{"NO Path Specified Yet!!"};

private:
    std::optional<torch::jit::script::Module> MonotonicModelV1{std::nullopt}};
    std::optional<torch::jit::script::Module> VAE1_InputLayerEncoder{std::nullopt}};
    std::optional<torch::jit::script::Module> VAE1_Encoder{std::nullopt};
    std::optional<torch::jit::script::Module> VAE1_LatentEncoder{std::nullopt};
    std::optional<torch::jit::script::Module> VAE1_Decoder{std::nullopt};

    int time_steps;
    int num_voices;
    torch::Tensor hits_logits;
    torch::Tensor hits_probabilities;
    torch::Tensor hits;
    torch::Tensor velocities;
    torch::Tensor offsets;
    torch::Tensor per_voice_sampling_thresholds;            // per voice thresholds for sampling
    vector<float> per_voice_max_count_allowed;            // per voice Maximum limit of hits
    float sampling_temperature {1.0f};

};


std::string tensor2string (torch::Tensor tensor);
inline torch::jit::script::Module LoadModel(std::string model_path, bool eval = true);