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
    [t, x] = ode45(@(t,y) -1*systemDynamics(y, DragArea, DryMass, LaunchSitePressure, LaunchSiteTemperature), [0 MaxTime], [0,TargetApogee,HorizontalVelocity,0], odeset('Events', @(t,y) stopEvent(t,y,MaxVelocity)));
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

function [velocities, angles, altitudes] = generateRawMeshData(MinFinalHorizontalVelocity, MaxFinalHorizontalVelocity, HorizontalVelocityIncrement, MaxVelocity, TargetAltitude, Temp, Pressure, DryMass, DragArea)
    velocities = zeros(0);
    angles = zeros(0);
    altitudes = zeros(0);
    for(HorizontalVelocity = MinFinalHorizontalVelocity : HorizontalVelocityIncrement : MaxFinalHorizontalVelocity)
        [t, x] = ODETimeSolution(MaxVelocity, TargetAltitude, HorizontalVelocity, Temp, Pressure, DragArea, DryMass);
        [~,x] = reIndex(t,x);
        [pair, newAlt] = makeStateFormat(x);
        newVel = pair(:,1);
        newAng = pair(:,2);
        velocities = concatColAndPad(velocities, newVel);
        angles = concatColAndPad(angles, newAng);
        altitudes = concatColAndPad(altitudes, newAlt);
    end
end

function C = concatRowAndPad(A, B)
    if size(A, 1) < size(B, 1)
        A = [A; zeros(size(B,1)-size(A,1), size(A,2))];
    elseif size(B, 1) < size(A, 1)
        B = [B; zeros(size(A,1)-size(B,1), size(B,2))];
    end
    C = [A;B];
end

function C = concatColAndPad(A,B)
    if size(A,1) < size(B,1)
        A = [A; zeros(size(B,1) - size(A,1), size(A,2))];
    elseif size(B,1) < size(A,1)
        B = [B; zeros(size(A,1) - size(B,1), size(B,2))];
    end
    C = [A, B];
end

%Vehicle and environment properties
LaunchSitePressure = 101325; %Pa
LaunchSiteTemperature = 288.15; %K
TargetApogee = 800; %m
MinimumDragArea = 0.0025; %m^2
MaximumDragArea = 0.01; %m^2
DryMass = 3; %kg

%Mesh Properties
MaxVelocity = 150; %m/s
VelocitySamplePeriod = 1; %m/s per sample
MaxAngle = 45; %degrees
AngleSamplePeriod = 1; %degrees per sample
ExportFileName = "mesh.csv"; %file were the mesh is saved
DoDisplay = true; %diplay figures of mesh or not

%generation properties
MinFinalHorizontalVelocity = 0;
MaxFinalHorizontalVelocity = 60;
HorizontalVelocityIncrement = 5;

%plot
[v,ang, h] = generateRawMeshData(MinFinalHorizontalVelocity, MaxFinalHorizontalVelocity, HorizontalVelocityIncrement, MaxVelocity, TargetApogee, LaunchSiteTemperature, LaunchSitePressure, DryMass, MinimumDragArea);
[v1,ang1, h1] = generateRawMeshData(MinFinalHorizontalVelocity, MaxFinalHorizontalVelocity, HorizontalVelocityIncrement, MaxVelocity, TargetApogee, LaunchSiteTemperature, LaunchSitePressure, DryMass, MaximumDragArea);
figure(1)
clf
hold on
for(i=1:size(v,2))
    plot3(v(:,i), ang(:,i), h(:,i), 'b');
end
for(i=1:size(v1,2))
    plot3(v1(:,i), ang1(:,i), h1(:,i), 'r');
end
hold off










