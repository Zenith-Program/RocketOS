classdef FloatsToBytes < matlab.System

    properties (Nontunable)
        NumValues = 1;
    end

    methods(Access = protected)

        function bytes = stepImpl(obj, floatValues)
            bytes = typecast(double(floatValues), 'uint8')';
        end

        function out = getOutputSizeImpl(obj)
            out = [1 8*obj.NumValues]; 
        end

        function out = getOutputDataTypeImpl(obj)
            out = "uint8";
        end

        function out = isOutputComplexImpl(obj)
            out = false;
        end

        function out = isOutputFixedSizeImpl(obj)
            out = true;
        end

        function num = getNumInputsImpl(obj)
            num = 1;
        end

        function out = getInputNamesImpl(obj)
            out = 'floatValues';
        end

        function out = isInputFixedSizeImpl(obj)
            out = true;
        end

        function out = isInputComplexImpl(obj)
            out = false;
        end

        function out = getInputSizeImpl(obj)
            out = [1 obj.NumValues];
        end

        function out = getInputDataTypeImpl(obj)
            out = "float";
        end
    end
end