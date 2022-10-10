import os.path

import torch
from BaseGrooveTransformers.models.transformer import GrooveTransformerEncoder
import params as prm


def load_model_in_eval_mode(model_param_dict):

    # get model name
    model_path = model_param_dict['pyModelPath']

    # load checkpoint
    checkpoint = torch.load(model_path, map_location=model_param_dict['device'])

    # Initialize model
    groove_transformer = GrooveTransformerEncoder(model_param_dict['d_model'],
                                                  model_param_dict['embedding_sz'],
                                                  model_param_dict['embedding_sz'],
                                                  model_param_dict['n_heads'],
                                                  model_param_dict['dim_ff'],
                                                  model_param_dict['dropout'],
                                                  model_param_dict['n_layers'],
                                                  model_param_dict['max_len'],
                                                  model_param_dict['device'])

    # Load model and put in evaluation mode
    groove_transformer.load_state_dict(checkpoint['model_state_dict'])
    groove_transformer.eval()

    return groove_transformer


if __name__ == '__main__':

    for model_name_, model_param_dict_ in prm.model_params.items():
        groove_transformer = load_model_in_eval_mode(model_param_dict_)

        # Providing input and output names sets the display names for values
        # within the model's graph. Setting these does not change the semantics
        # of the graph; it is only for readability.
        #
        # The inputs to the network consist of the flat list of inputs (i.e.
        # the values you would pass to the forward() method) followed by the
        # flat list of parameters. You can partially specify names, i.e. provide
        # a list here shorter than the number of inputs to the model, and we will
        # only set that subset of names, starting from the beginning.
        input_names = [ "groove_input" ]
        output_names = [ "h_logits", "velocity", "offset"  ]

        # trace model for serialization
        # https://pytorch.org/tutorials/advanced/cpp_export.html
        example = torch.rand(1, 32, 27)
        torch.onnx.export(groove_transformer, example, os.path.join("serializedONNX", model_name_+".onnx"), verbose=True, input_names=input_names, output_names=output_names)
