function dragArea = getDragArea(acceleration, velocity, altitude, angle, mass, p0, t0)
    g = 9.8;
    dragArea = -2 .* mass .* sin(angle) ./ (densityFromAltitude(altitude, p0, t0) .* velocity .* velocity) .* (acceleration + g);
end

function density = densityFromAltitude(altitude, p0, T0)
    % Constants
    L = 0.0065;      % K/m temperature lapse rate
    g = 9.80665;     % m/s^2 gravitational constant
    R = 8.31446;     % J/(mol*K) Ideal gas constant
    M = 0.0289652;   % kg/mol Molar mass of air
    density = p0*M/(R*T0)*(1-L*altitude/T0).^(g*M/(R*L)-1);
end

function plotStateTransitions(stateNames, time)
    for(i=1:length(time)-1)
        if(strcmp(stateNames(i), stateNames(i+1)) == 0)
            xline(time(i), '--r');
        end
    end
end

% === script parameters ===
TelemetryFileName = "telemetry.csv";
FlightPlanFileName = "flightPath.csv";

%read telemetry data
Data = readmatrix(TelemetryFileName);
Timestamp = Data(:,1);
Timestamp = (Timestamp - Timestamp(1))/1000;
PredictedAltitude = Data(:,3);
PredictedVerticalVelocity = Data(:,4);
PredictedVerticalAcceleratrion = Data(:,5);
PredictedAngleToHorizontal = Data(:,6);
MeasuredAltitude = Data(:,7);
MeasuredPressure = Data(:,8);
MeasuredTemperature = Data(:,9);
MeasuredAcceleration = Data(:,10:12);
MeasuredRotation = Data(:,13:15);
MeasuredGravity = Data(:,16:18);
MeasuredOrientation = Data(:,19:22);
MeasuredAngleToHorizontal = Data(:,23);
Error = Data(:,24);
FlightPath = Data(:,25);
FlightPathDerivative = Data(:,26:27);
UpdateRuleDrag = Data(:,28);
AdjustedDrag = Data(:,29);
RequestedDrag = Data(:,30);
CurrentDrag = Data(:,31);
table = readtable(TelemetryFileName,'ReadVariableNames', true);
StateNames = string(table{:, 2});

%flight plan
Plan = readmatrix(FlightPlanFileName);
content = fileread(FlightPlanFileName);
lines = splitlines(content);
str = lines(1); % Adjust to read desired number of metadata lines
cell_array = split(str, ',');
PlanMeta = str2double(cell_array);
TargetApogee = PlanMeta(1);
MinDragArea = PlanMeta(2);
MaxDragArea = PlanMeta(3);
MaxDeploymentAngle = PlanMeta(4); %degrees
LaunchSitePressure = PlanMeta(7); %Pa
LaunchSiteTemperature = PlanMeta(6); %K
VehicleMass = PlanMeta(5); %kg
MaxVelocity = PlanMeta(8);
VelocitySamples = PlanMeta(9);
AngleSamples = PlanMeta(9);

%altitudes
figure(1)
clf
hold on
plot(Timestamp, PredictedAltitude);
plot(Timestamp, MeasuredAltitude);
plot(Timestamp, FlightPath);
plot(Timestamp, PredictedVerticalVelocity);
plot(Timestamp, PredictedVerticalAcceleratrion);
plotStateTransitions(StateNames, Timestamp);
hold off
xlabel("Time (s)");
ylabel("Altitude (m)");
title("Airbrakes Flight Profile");
legend('Predicted Altitude','Measured Altitude','Flight Plan','Vertical Velocity (m/s)','Vertical Acceleration (m/s^2)', 'State Transition')

%drag
figure(2)
clf
hold on
plot(Timestamp, UpdateRuleDrag);
plot(Timestamp, AdjustedDrag);
plot(Timestamp, RequestedDrag);
plot(Timestamp, CurrentDrag);
plot(Timestamp, getDragArea(PredictedVerticalAcceleratrion, PredictedVerticalVelocity, PredictedAltitude, PredictedAngleToHorizontal, VehicleMass, LaunchSitePressure, LaunchSiteTemperature));
plot(Timestamp, ones(1, length(Timestamp)) * MinDragArea, '--')
plot(Timestamp, ones(1, length(Timestamp)) * MaxDragArea, '--')
plotStateTransitions(StateNames, Timestamp);
hold off
xlabel("Time (s)");
ylabel("Effective Drag Area (m^2)");
title("Airbrakes Drag Analysis");
legend("Update Rule Drag", "Adjusted Drag", "Requested Drag", "Actuator Position Drag", "Measured Drag", "Minimum Drag Area", "Maximum Drag Area", "State Transition");
ylim([MinDragArea - (MaxDragArea - MinDragArea)/4, MaxDragArea + (MaxDragArea - MinDragArea)/4]);

%flight plan
figure(3)
clf
hold on
plot3(PredictedVerticalVelocity, PredictedAngleToHorizontal * 180 / pi, FlightPath);
plot3(PredictedVerticalVelocity, PredictedAngleToHorizontal * 180 / pi, PredictedAltitude);
pathVelocities = linspace(MaxVelocity, 0, VelocitySamples);
pathAngles = linspace(pi/2, 0, AngleSamples);
s = surface(pathVelocities, pathAngles * 180/pi, centerPathAltitudes);
s.EdgeColor = 'k';
s.FaceColor = 'k';
s.FaceAlpha = 0.25;
s.EdgeAlpha = 0;
scatter3(0,0,TargetApogee, 'filled' ,'k');
text(0,0,TargetApogee, "Target Apogee");
hold off
xlabel("Vertical Velocity (m/s)");
ylabel("Angle to Horizontal (degrees)");
zlabel("Altitude (m)");
title("Airbrakes Flight Path");
legend("flight plan reference", "flight path", "flight plan mesh");

%error
figure(4)
clf
plot(Timestamp, Error);
plotStateTransitions(StateNames, Timestamp);
yline(0, "k--");
xlabel("Time (s)");
ylabel("Error");
title("Airbrakes Controller Error");
legend("Error", "State Transition");
