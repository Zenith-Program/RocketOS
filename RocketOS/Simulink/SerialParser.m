classdef SerialParser < matlab.System
    % Interpreted mode only. Outputs a vector of NumValues doubles
    % Example input: "#1.0 2.0 3.0\n" → [1.0 2.0 3.0]

    properties
        Port = "COM3";
        BaudRate = 115200;
        NumValues = 3;  % Change this to adjust output length
    end

    properties(Access = private)
        sObj
        lastValues
    end

    methods(Access = protected)
        function setupImpl(obj)
            obj.sObj = serialport(obj.Port, obj.BaudRate);
            configureTerminator(obj.sObj, "LF");
            flush(obj.sObj);
            obj.lastValues = zeros(1, obj.NumValues);
        end

        function y = stepImpl(obj)
            y = obj.lastValues;

            while obj.sObj.NumBytesAvailable > 0
                line = readline(obj.sObj);
                if startsWith(line, "#")
                    nums = sscanf(extractAfter(line, 1), '%f');
                    if numel(nums) >= obj.NumValues
                        y = nums(1:obj.NumValues)';
                        obj.lastValues = y;
                    end
                end
            end
        end

        function resetImpl(obj)
            obj.lastValues = zeros(1, obj.NumValues);
        end

        function releaseImpl(obj)
            clear obj.sObj;
        end

        function out = getOutputSizeImpl(obj)
            out = [1 obj.NumValues];  % Row vector output
        end

        function out = getOutputDataTypeImpl(obj)
            out = "double";
        end

        function out = isOutputComplexImpl(obj)
            out = false;
        end

        function out = isOutputFixedSizeImpl(obj)
            out = true;
        end
    end
end