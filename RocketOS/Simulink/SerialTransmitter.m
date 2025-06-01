classdef SerialTransmitter < matlab.System
    properties
        Port = "COM3";
        BaudRate = 115200;
    end

    properties(Access = private)
        sObj
    end

    methods(Access = protected)
        function setupImpl(obj)
            obj.sObj = serialport(obj.Port, obj.BaudRate);
        end

        function stepImpl(obj, dataVec, commandStr)
            % Priority: send command if non-empty
            if ~isempty(strtrim(commandStr))
                writeline(obj.sObj, commandStr);
            elseif ~isempty(dataVec)
                formatted = sprintf('#%g ', dataVec);
                writeline(obj.sObj, strtrim(formatted)); % adds \n
            end
        end

        function releaseImpl(obj)
            clear obj.sObj;
        end

        function resetImpl(~)
        end

        function num = getNumInputsImpl(~)
            num = 2;
        end

        function out = getInputNamesImpl(~)
            out = {'dataVec', 'commandStr'};
        end
    end
end