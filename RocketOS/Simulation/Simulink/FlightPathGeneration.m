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

function [val, terminal, dir] = stopEvent(t,y,maxVelocity)
    t;
    val = y(4) - maxVelocity;
    terminal = 1;
    dir = 0;
end

function [t,x] = ODETimeSolution(MaxVelocity, TargetApogee, HorizontalVelocity, LaunchSiteTemperature, LaunchSitePressure, DragArea, DryMass)
    MaxTime = 100;
    [t, x] = ode45(@(t,y) -1*systemDynamics(y, DragArea, DryMass, LaunchSitePressure, LaunchSiteTemperature), 0 : 0.125 : MaxTime, [0,TargetApogee,HorizontalVelocity,0], odeset('Events', @(t,y) stopEvent(t,y,MaxVelocity)));
end

function [t_new, x_new] = reIndex(t,x)
    t_max = max(t);
    t_new = flip(abs(t-t_max));
    x_new = flip(x);
end

function [VelocityAnglePair, Altitude] = makeStateFormat(x)
    VelocityAnglePair = [x(:,4), atan(x(:,4)./x(:,3))];
    Altitude = x(:,2);
end

function [velocities, angles, altitudes] = generateRawMeshData(FinishAngle, HorizontalVelocityIncrement, MaxVelocity, TargetAltitude, Temp, Pressure, DryMass, DragArea)
    velocityFilVal = 0;
    angleFillVal = 0;
    altitudeFillVal = TargetAltitude;
    velocities = zeros(0);
    angles = zeros(0);
    altitudes = zeros(0);
    HorizontalVelocity = 0;
    startAngle = pi/2;
    while(startAngle > FinishAngle)
        [t, x] = ODETimeSolution(MaxVelocity, TargetAltitude, HorizontalVelocity, Temp, Pressure, DragArea, DryMass);
        [~,x] = reIndex(t,x);
        [pair, newAlt] = makeStateFormat(x);
        newVel = pair(:,1);
        newAng = pair(:,2);
        velocities = concatColAndPad(velocities, newVel, velocityFilVal);
        angles = concatColAndPad(angles, newAng, angleFillVal);
        altitudes = concatColAndPad(altitudes, newAlt, altitudeFillVal);
        startAngle = max(newAng);
        HorizontalVelocity = HorizontalVelocity + HorizontalVelocityIncrement;
    end
end

function C = concatColAndPad(A,B,val)
    if size(A,1) < size(B,1)
        A = [A; val*ones(size(B,1) - size(A,1), size(A,2))];
    elseif size(B,1) < size(A,1)
        B = [B; val*ones(size(A,1) - size(B,1), size(B,2))];
    end
    C = [A, B];
end

function [v,angle, h] = addSingularPath(V, ANGLE, H, MaxVelocity, TargetApogee)
    numSamples = size(V,1);
    v = [V linspace(MaxVelocity, 0, numSamples)'];
    angle = [ANGLE zeros(numSamples, 1)];
    h = [H TargetApogee*ones(numSamples, 1)];
end

function [angle] = removeNAN(ANGLE)
    for(i=1:size(ANGLE,1))
        for(j=1:size(ANGLE,2))
            if(isnan(ANGLE(i,j))) 
                ANGLE(i,j) = pi/2;
            end
        end
    end
    angle = ANGLE;
end

function [velocities, angles, altitudes] = resampleVelocities(Velocities, Angles, Altitudes, numSamples, MaxVelocity)
    velocities = zeros(size(Velocities,2), numSamples);
    angles = zeros(size(Velocities,2), numSamples);
    altitudes = zeros(size(Velocities,2), numSamples);
    newSamples = linspace(MaxVelocity, 0, numSamples);
    for(i=1:size(Velocities,2))
        for(j=1:length(newSamples))
            newVelocity = newSamples(j);
            lastGreaterThanIndex = sum(cumprod(newVelocity<Velocities(:,i)));
            if(lastGreaterThanIndex == 0)
                %new sample is before first sample
                velocities(i,j) = newVelocity;
                angles(i,j) = Angles(1,i);
                altitudes(i,j) = Altitudes(1,i);
            elseif(lastGreaterThanIndex == size(Velocities,1))
                %new sample is after last sample
                velocities(i,j) = newVelocity;
                angles(i,j) = Angles(size(Angles,1),i);
                altitudes(i,j) = Altitudes(size(Altitudes,1),i);
            else
                %new sample is between two samples
                thisSample = Velocities(lastGreaterThanIndex, i);
                nextSample = Velocities(lastGreaterThanIndex+1, i);
                velocities(i,j) = newVelocity;
                angles(i,j) = linInterp(thisSample, Angles(lastGreaterThanIndex, i), nextSample, Angles(lastGreaterThanIndex+1, i), newVelocity);
                altitudes(i,j) = linInterp(thisSample, Altitudes(lastGreaterThanIndex,i), nextSample, Altitudes(lastGreaterThanIndex+1, i), newVelocity);
            end
        end
    end
end

function [velocities, angles, altitudes] = resampleAngles(Velocities, Angles, Altitudes, NumSamples)
    velocities = zeros(NumSamples, size(Velocities,2));
    angles = zeros(NumSamples, size(Velocities,2));
    altitudes = zeros(NumSamples, size(Velocities, 2));
    newSamples = linspace(pi/2, 0, NumSamples);
    for(i=1:size(Velocities,2))
        for(j=1:length(newSamples))
            newAngle = newSamples(j);
            lastGreaterThanIndex = sum(cumprod(newAngle<Angles(:,i)));
            if(lastGreaterThanIndex == 0)
                %new sample is before first sample
                velocities(j,i) = Velocities(1,i);
                angles(j,i) = newAngle;
                altitudes(j,i) = Altitudes(1,i);
            elseif(lastGreaterThanIndex == size(Velocities,1))
                %new sample is after last sample
                velocities(j,i) = velocities(size(Velocities, 2),i);
                angles(j,i) = newAngle;
                altitudes(j,i) = Altitudes(size(Altitudes,2),i);
            else
                %new sample is between two samples
                thisSample = Angles(lastGreaterThanIndex, i);
                nextSample = Angles(lastGreaterThanIndex+1, i);
                velocities(j,i) = linInterp(thisSample, Velocities(lastGreaterThanIndex,i), nextSample, Velocities(lastGreaterThanIndex+1,i), newAngle);
                angles(j,i) = newAngle;
                altitudes(j,i) = linInterp(thisSample, Altitudes(lastGreaterThanIndex,i), nextSample, Altitudes(lastGreaterThanIndex+1,i), newAngle);
            end
        end
    end

end

function y = linInterp(x1,y1,x2,y2,x)
    if(x2-x1 == 0)
        y = y1;
    else
        y = (y2-y1)/(x2-x1)*(x-x1)+y1;
    end
end

function saveFlightPath(file, altitudes, targetApogee, minDragArea, maxDragArea, angleLim, mass, temp, pressure, maxVelocity, vSize, angleSize)
    header = [targetApogee minDragArea maxDragArea angleLim mass temp pressure maxVelocity vSize angleSize];
    writematrix(header, file, 'WriteMode','overwrite');
    writematrix(altitudes, file, 'WriteMode','append');
end

function [v, angle] = gradientMesh(altitudes, vMax, numV, numAngle)   
vIncrement = vMax / numV;
angleIncrement = pi/2 / numAngle;
v = zeros(size(altitudes));
angle = zeros(size(altitudes));
    for(i=1:numV)
        for(j=1:numAngle)
            %compute v diraction
            if(j == 1)
                v(i,j) = (altitudes(i, j) - altitudes(i,j+1))/vIncrement;
            elseif(j==numV)
                v(i,j) = (altitudes(i, j-1) - altitudes(i,j))/vIncrement;
            else
                v(i,j) = (altitudes(i, j-1) - altitudes(i, j+1))/(2*vIncrement);
            end
            %compute angle direction
            if(i == 1)
                angle(i,j) = (altitudes(i,j) - altitudes(i+1, j)) / angleIncrement;
            elseif(i==numAngle)
                angle(i,j) = (altitudes(i-1,j) - altitudes(i, j)) / angleIncrement;
            else
                angle(i,j) = (altitudes(i-1,j) - altitudes(i+1,j)) / (2*angleIncrement);
            end
        end
    end
end

%Vehicle and environment properties
LaunchSitePressure = 101325; %Pa
LaunchSiteTemperature = 288.15; %K
TargetApogee = 800; %m
MinimumDragArea = 0.0025; %m^2
MaximumDragArea = 0.01; %m^2
ActuatorDeploymentAngleLimit = 90; %degrees
DryMass = 3; %kg

%Mesh Properties
MaxVelocity = 150; %m/s
VelocitySamples = 64; %number of velocity samples
MinAngle = 0.1; % where to stop before singularity degrees
AngleSamples = 64; %degrees per sample
ExportFileName = "flightPath.csv"; %file were the mesh is saved
DoDisplay = true; %diplay figures of mesh or not

%generation properties
HorizontalVelocityIncrement = 1;

%Generate Mesh
[lowerPathVelocities,lowerPathAngles, lowerPathAltitudes] = generateRawMeshData(MinAngle * pi/180, HorizontalVelocityIncrement, MaxVelocity, TargetApogee, LaunchSiteTemperature, LaunchSitePressure, DryMass, MinimumDragArea);
[upperPathVelocities,upperPathAngles, upperPathAltitudes] = generateRawMeshData(MinAngle * pi/180, HorizontalVelocityIncrement, MaxVelocity, TargetApogee, LaunchSiteTemperature, LaunchSitePressure, DryMass, MaximumDragArea);
[lowerPathVelocities, lowerPathAngles, lowerPathAltitudes] = addSingularPath(lowerPathVelocities, lowerPathAngles, lowerPathAltitudes, MaxVelocity, TargetApogee);
[upperPathVelocities, upperPathAngles, upperPathAltitudes] = addSingularPath(upperPathVelocities, upperPathAngles, upperPathAltitudes, MaxVelocity, TargetApogee);
lowerPathAngles = removeNAN(lowerPathAngles);
upperPathAngles = removeNAN(upperPathAngles);

%plot intermediate surfaces (if enabled)

[lowerPathVelocities,lowerPathAngles, lowerPathAltitudes] = resampleVelocities(lowerPathVelocities, lowerPathAngles, lowerPathAltitudes, VelocitySamples, MaxVelocity);
[upperPathVelocities,upperPathAngles, upperPathAltitudes] = resampleVelocities(upperPathVelocities, upperPathAngles, upperPathAltitudes, VelocitySamples, MaxVelocity);

%plot intermediate surfaces (if enabled)

[lowerPathVelocities,lowerPathAngles, lowerPathAltitudes] = resampleAngles(lowerPathVelocities, lowerPathAngles, lowerPathAltitudes,AngleSamples);
[upperPathVelocities,upperPathAngles, upperPathAltitudes] = resampleAngles(upperPathVelocities, upperPathAngles, upperPathAltitudes,AngleSamples);

%compute centerline path
centerPathAltitudes = (lowerPathAltitudes + upperPathAltitudes)/2;

%plot past cone
figure(2)
clf
hold on
s = surface(upperPathVelocities, upperPathAngles * 180/pi, upperPathAltitudes);
s.EdgeColor = 'r';
s.FaceColor = 'r';
s.FaceAlpha = 0.75;
s.EdgeAlpha = 0.75;
s = surface(lowerPathVelocities, lowerPathAngles * 180/pi, lowerPathAltitudes);
s.EdgeColor = 'b';
s.FaceColor = 'b';
s.FaceAlpha = 0.75;
s.EdgeAlpha = 0.75;
scatter3(0,0,TargetApogee, 'filled' ,'k');
text(0,0,TargetApogee, "Target Apogee");
hold off
title("Target's Past Cone");
xlabel('Vertical Velocity (m/s)');
ylabel("Angle (degrees)");
zlabel("Altitude (m)");

%plot centerline path
figure(3)
clf
hold on
s = surface(upperPathVelocities, upperPathAngles * 180/pi, upperPathAltitudes);
s.EdgeColor = 'k';
s.FaceColor = 'k';
s.FaceAlpha = 0.25;
s.EdgeAlpha = 0.25;
s = surface(lowerPathVelocities, lowerPathAngles * 180/pi, lowerPathAltitudes);
s.EdgeColor = 'k';
s.FaceColor = 'k';
s.FaceAlpha = 0.25;
s.EdgeAlpha = 0.25;
s = surface(lowerPathVelocities, lowerPathAngles * 180/pi, centerPathAltitudes);
s.EdgeColor = 'k';
s.FaceColor = 'g';
scatter3(0,0,TargetApogee, 'filled' ,'k');
text(0,0,TargetApogee, "Target Apogee");
hold off
title("Centerline Flight Path with Target's Past Cone");
xlabel('Vertical Velocity (m/s)');
ylabel("Angle (degrees)");
zlabel("Altitude (m)");

%save to file
saveFlightPath(ExportFileName, centerPathAltitudes, TargetApogee, MinimumDragArea, MaximumDragArea, ActuatorDeploymentAngleLimit, DryMass, LaunchSiteTemperature, LaunchSitePressure, MaxVelocity, VelocitySamples, AngleSamples);

%For simulink modeling
SimMesh = centerPathAltitudes(end:-1:1, end:-1:1)';
SimVelocitySamples = linspace(0, MaxVelocity, VelocitySamples);
SimAngleSamples = linspace(0, pi/2, AngleSamples);

[SimGradVMesh, SimGradAngleMesh] = gradientMesh(centerPathAltitudes, MaxVelocity, VelocitySamples, AngleSamples);

%plot gradients & path
VectorFieldSkips = 4;
figure(4)
clf
hold on
s = surface(lowerPathVelocities, lowerPathAngles * 180/pi, centerPathAltitudes);
s.EdgeColor = 'k';
s.FaceColor = 'g';
scatter3(0,0,TargetApogee, 'filled' ,'k');
text(0,0,TargetApogee, "Target Apogee");
quiver3(lowerPathVelocities(1:VectorFieldSkips:end, 1:VectorFieldSkips:end), lowerPathAngles(1:VectorFieldSkips:end, 1:VectorFieldSkips:end) * 180/pi, zeros(size(centerPathAltitudes(1:VectorFieldSkips:end, 1:VectorFieldSkips:end))), SimGradVMesh(1:VectorFieldSkips:end, 1:VectorFieldSkips:end), SimGradAngleMesh(1:VectorFieldSkips:end, 1:VectorFieldSkips:end) * pi/180, zeros(size(centerPathAltitudes(1:VectorFieldSkips:end, 1:VectorFieldSkips:end))), 'k');
hold off
title("Centerline Flight Path with Gradient Field");
xlabel('Vertical Velocity (m/s)');
ylabel("Angle (degrees)");
zlabel("Altitude (m)");

%just flight plan mesh
figure(5)
clf
hold on
s = surface(lowerPathVelocities, lowerPathAngles * 180/pi, centerPathAltitudes);
s.EdgeColor = 'k';
s.FaceColor = 'g';
scatter3(0,0,TargetApogee, 'filled' ,'k');
text(0,0,TargetApogee, "Target Apogee");
title("Airbrakes Flight Plan");
xlabel('Vertical Velocity (m/s)');
ylabel("Angle (degrees)");
zlabel("Altitude (m)");
hold off
zlim([0,900])

SimGradVMesh = SimGradVMesh(end:-1:1, end:-1:1)';
SimGradAngleMesh = SimGradAngleMesh(end:-1:1, end:-1:1)';





