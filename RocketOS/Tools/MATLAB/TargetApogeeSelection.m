function x_dot = systemDynamics(x, u, m, p0, T0)
    g = 9.8;
    x_dot = zeros(4,1);
    x_dot(1) = x(3);
    x_dot(2) = x(4);
    x_dot(3) = 1/m * (-1/2 * densityFromAltitude(x(2), p0, T0)*u*sqrt(x(3)^2 + x(4)^2)*x(3));
    x_dot(4) = 1/m * (-1/2 * densityFromAltitude(x(2), p0, T0)*u*sqrt(x(3)^2 + x(4)^2)*x(4) - m * g);
end

function density = densityFromAltitude(altitude, p0, T0)
    % Constants
    L = 0.0065;      % K/m temperature lapse rate
    g = 9.80665;     % m/s^2 gravitational constant
    R = 8.31446;     % J/(mol*K) Ideal gas constant
    M = 0.0289652;   % kg/mol Molar mass of air
    density = p0*M/(R*T0)*(1-L*altitude/T0).^(g*M/(R*L)-1);
end

function [val, terminal, dir] = stopEvent(t,y)
    t;
    val = y(4);
    terminal = 1;
    dir = 0;
end

function [t,x] = ODETimeSolution(InitialVelocity, InitialAltitude, InitialAngle, LaunchSiteTemperature, LaunchSitePressure, DragArea, DryMass)
    MaxTime = 100;
    [t, x] = ode45(@(t,y) systemDynamics(y, DragArea, DryMass, LaunchSitePressure, LaunchSiteTemperature), 0 : 0.125 : MaxTime, [0,InitialAltitude,InitialVelocity*cos(InitialAngle),InitialVelocity*sin(InitialAngle)], odeset('Events', @(t,y) stopEvent(t,y)));
end

function [angles, altitudes] = generateApogeeData(NumAngles, InitialVelocity, InitialAltitude, Temp, Pressure, DryMass, DragArea)
    angles = linspace(0, pi/2, NumAngles);
    altitudes = zeros(NumAngles, 1);
    altitudes(1) = InitialAltitude;
    for(i = 2:NumAngles)
        startAngle = angles(i);
        [~, x] = ODETimeSolution(InitialVelocity, InitialAltitude, startAngle, Temp, Pressure, DragArea, DryMass);
        [~, newAlt] = makeStateFormat(x);
        altitudes(i) = newAlt(length(newAlt));
    end
end

function [VelocityAnglePair, Altitude] = makeStateFormat(x)
    VelocityAnglePair = [x(:,4), atan(abs(x(:,4)./x(:,3)))];
    Altitude = x(:,2);
end

function [angles, density] = angleProbabilityDensity(railVelocity, railAngle, windVelocity, windVariance, NumAngles)
    angles = linspace(0, pi/2, NumAngles);
    density = normpdf(railVelocity*sin(railAngle) ./ tan(angles) - railVelocity * cos(railAngle), windVelocity, windVariance) .* railVelocity .* sin(railAngle) .* csc(angles).*csc(angles) + normpdf(railVelocity*sin(railAngle) ./ tan(-angles) - railVelocity * cos(railAngle), windVelocity, windVariance) .* railVelocity .* sin(railAngle) .* csc(-angles).*csc(-angles);
end

function [altitudes, probabilities] = targetApogeeProbabilities(MinAltitude, MaxAltitude, NumAltitudes, railVelocity, railAngle, windVelocity, windVariance, NumAngles, InitialVelocity, InitialAltitude, Temp, Pressure, DryMass, MinDragArea, MaxDragArea)
    [~, minApogees] = generateApogeeData(NumAngles, InitialVelocity, InitialAltitude, Temp, Pressure, DryMass, MinDragArea);
    [~, maxApogees] = generateApogeeData(NumAngles, InitialVelocity, InitialAltitude, Temp, Pressure, DryMass, MaxDragArea);
    [angles, angleProbabilities] = angleProbabilityDensity(railVelocity, railAngle, windVelocity, windVariance, NumAngles);
    altitudes = linspace(MinAltitude, MaxAltitude, NumAltitudes);
    probabilities = zeros(length(altitudes), 1);
    dAngle = angles(2) - angles(1);
    for(i=1:length(altitudes))
        for(j=1:length(angles))
            if(altitudes(i) > maxApogees(j) && altitudes(i) < minApogees(j))
                probabilities(i) = probabilities(i) + dAngle * angleProbabilities(j);
            end
        end
    end
    norm = 0;
    for(j=1:length(angles))
        if(~isnan(angleProbabilities(j)))
            norm = norm + dAngle * angleProbabilities(j);
        end
    end
    probabilities = probabilities / norm;
end


function y = linInterp(x1,y1,x2,y2,x)
    if(x2-x1 == 0)
        y = y1;
    else
        y = (y2-y1)/(x2-x1)*(x-x1)+y1;
    end
end

% === Script Parameters ===
InitialVelocity = 150; %m/s
InitialAltitude = 100; %m
Mass = 6; %
AngleSamples = 500;
LaunchSitePressure = 101325; %Pa
LaunchSiteTemperature = 288.15; %K
MinimumDragArea = 0.0025; %m^2
MaximumDragArea = 0.01; %m^2
MinimumTargetApogee = 300;
MaximumTargetApogee = 1100;
ApogeeSamples = 200; 
RailVelocity = 30; %m/s
RailAngle = 5; %degrees
WindSpeed = 7; %m/s
WindSpeedStdDev = 3;

RailAngleRad = pi/2 - RailAngle * pi / 180;

[angle,h] = generateApogeeData(AngleSamples, InitialVelocity, InitialAltitude, LaunchSiteTemperature, LaunchSitePressure, Mass, MaximumDragArea);
[angle1,h1] = generateApogeeData(AngleSamples, InitialVelocity, InitialAltitude, LaunchSiteTemperature, LaunchSitePressure, Mass, MinimumDragArea);
X_fill = [angle(1:length(angle)-1) * 180/pi, flip(angle(1:length(angle)-1)) * 180/pi];
Y_fill = [h(1:length(angle)-1)', flip(h1(1:length(angle)-1)')];
figure(1)
clf
hold on
plot(angle * 180/pi,h);
plot(angle1 * 180/pi,h1);
fill(X_fill, Y_fill, 'k', 'FaceAlpha', 0.3, 'EdgeColor', 'none')
hold off
xlabel("Coast Angle (degrees)");
ylabel("Apogee (m)");
title("Possible Apogees for Initial Coast Angles");
legend("Maximum Drag Sustained", "Minimum Drag Sustained", "Set of Possible Apogees");

figure(2)
clf
[x,y] = angleProbabilityDensity(RailVelocity,RailAngleRad,-WindSpeed,WindSpeedStdDev,AngleSamples);
plot(x * 180/pi,y);
xlabel("Coast Angle (degrees)");
ylabel("Probability Density");
title("Probability Distribution for Initial Coast Angles");

figure(3)
clf
[altitudes, probs] = targetApogeeProbabilities(MinimumTargetApogee, MaximumTargetApogee, ApogeeSamples, RailVelocity, RailAngleRad, -WindSpeed, WindSpeedStdDev, AngleSamples, InitialVelocity, InitialAltitude, LaunchSiteTemperature, LaunchSitePressure, Mass, MinimumDragArea, MaximumDragArea);
plot(altitudes, probs);
title("Probability of Reachability for Target Apogees");
xlabel("Target Apogee (m)");
ylabel("Probability");
ylim([0,1.1])
