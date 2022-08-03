//
// Created by Behzad Haki on 2022-08-03.
//

#ifndef JUCECMAKEREPO_MONOTONICGROOVETRANSFORMERV1_H
#define JUCECMAKEREPO_MONOTONICGROOVETRANSFORMERV1_H

#include <torch/script.h> // One-stop header.
using namespace torch::indexing;


class MonotonicGrooveTransformerV1
{
public:
    // constructor
    MonotonicGrooveTransformerV1(std::string model_path, int time_steps, int num_voices);

    // getters
    torch::Tensor get_hits_logits();
    torch::Tensor get_hits_probabilities();
    torch::Tensor get_velocities();
    torch::Tensor get_offsets();

    // setters
    void set_sampling_thresholds(torch::Tensor per_voice_thresholds);

    // Step 1. Passes input through the model and updates logits, vels and offsets
    void forward_pass(torch::Tensor monotonicGrooveInput);
    // Step 2. Sample hvo after the forward pass
    std::tuple<torch::Tensor, torch::Tensor, torch::Tensor> sample(std::string sample_mode = "Threshold");

private:

    torch::jit::script::Module model;           // model to be loaded in constructor
    int time_steps;
    int num_voices;
    torch::Tensor hits_logits;
    torch::Tensor hits_probabilities;
    torch::Tensor hits;
    torch::Tensor velocities;
    torch::Tensor offsets;
    torch::Tensor per_voice_sampling_thresholds;            // per voice thresholds for sampling


};


std::string tensor2string (torch::Tensor tensor);
inline torch::jit::script::Module LoadModel(std::string model_path, bool eval = true);

#endif //JUCECMAKEREPO_MONOTONICGROOVETRANSFORMERV1_H
