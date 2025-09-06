%airbrakes vehicle properties
uMax = 0.01; %m^2
uMin = 0.73 * 49.32 / 10000; %cm^2
mass = 6; %kg
maxVelocity = 150; %m/s
initialVelocity = 146; %m/s
initialAltitude = 100; %m
airDensity = 1.28; %
g = 9.8; %m/s



%functions
function k = kValue(airDensity, mass, dragArea)
    k = airDensity * dragArea / (2 * mass);
end


function h = altitudeTime(time, initialState, k)
    g = 9.8; %m/s
    h = initialState(2) + 1/k * log(cos(sqrt(k*g) * time) + initialState(1) * sqrt(k/g) * sin(sqrt(k * g) * time));
end

function v = velocityTime(time, initialState, k)
    g = 9.8; %m/s
    v = sqrt(g/k) * tan(atan(initialState(1) * sqrt(k/g) - sqrt(k * g) * time));
end

function t = apogeeTime(initialState, k)
    g = 9.8; %m/s
    t = atan(initialState(1) * sqrt(k/g))/sqrt(k * g);
end

function h = altitudeVelocity(velocity, initialState, k)
    g = 9.8; %m/s
    h = initialState(2) + 1/(2 * k) * log((g + k * initialState(1) * initialState(1)) ./ ( g + k * velocity .* velocity));
end

function h = integralFromula(velocity, initialState, u, density, mass)
    g = 9.8; %m/s
    k = @(v) u(v) * density / (2 * mass);
    integrand = @(v) -v ./ (g + k(v) .* v .* v);
    altitudes = zeros(length(velocity), 1);
    for(i = 1:length(altitudes))
        altitudes(i) = initialState(2) + integral(integrand, initialState(1), velocity(i));
    end

    h = altitudes;
end

%3d
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
    [t, x] = ode45(@(t,y) systemDynamics(y, DragArea, DryMass, LaunchSitePressure, LaunchSiteTemperature), 0 : 0.125 : MaxTime, [0,InitialAltitude,InitialVelocity*cot(InitialAngle),InitialVelocity], odeset('Events', @(t,y) stopEvent(t,y)));
end

function [VelocityAnglePair, Altitude] = makeStateFormat(x)
    VelocityAnglePair = [x(:,4), atan(abs(x(:,4)./x(:,3)))];
    Altitude = x(:,2);
end

%helpers
function s = randomSignal(x, min, max, a, omega)
    scale = max - min;
    s = 0;
    for(i = 1:length(a))
        s = s + a(i) * cos(i * omega * x);
    end
    s = s / length(a) * scale;
    s = s + min + (max - min)/2;
end

function formatIEEE()
    width = 3.5;   % inches, IEEE single column
    height = 2.6;  % adjust for aspect ratio
    set(gcf, 'Units', 'inches', 'Position', [1 1 width height]);
    set(gca, 'FontSize', 8, 'FontName', 'Times New Roman');
    set(findall(gca, 'Type', 'Line'), 'LineWidth', 1.0);
end

function [X_fill, Y_fill] = makeFillRegion(x1, y1, x2, y2)
    X_fill = [x1(1:length(x1)), flip(x2(1:length(x2)))];
    Y_fill = [y1(1:length(x1)), flip(y2(1:length(x2)))];
end

%comparison to open rocket
figure(1)
clf
k = kValue(airDensity, mass, uMin);
initial = [initialVelocity, initialAltitude];
times = linspace(0, apogeeTime(initial, k), 100);
altitudes = altitudeTime(times, initial, k);
velocities = velocityTime(times, initial, k);
hold on
plot(times, altitudes, 'k')
plot(times, velocities, '-.k')
scatter(times(100), altitudes(100), 'k', 'filled')
text(times(100), altitudes(100), "890.51m ", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'right', 'VerticalAlignment', 'bottom')
hold off

formatIEEE();
title("Flight Profile Predicted by 2D Model")
xlabel("Time (s)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Altitude (m)", "Velocity(m/s)", 'Location', 'northwest', 'FontSize', 8)

%state space
figure(2)
clf
velocities = linspace(initialVelocity, 0, 100);
altitudes = altitudeVelocity(velocities, initial, k);
hold on
plot(velocities, altitudes, 'k')
scatter(0, altitudes(100), 'k', 'filled')
text(0, altitudes(100), " Apogee", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'bottom')
text(initialVelocity, initialAltitude, "Burnout ", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'right', 'VerticalAlignment', 'top')
scatter(initialVelocity, initialAltitude, 'k', 'filled')
hold off
ylim([0 1000])
xlim([0 150])
formatIEEE();
title("Predicted Flight Path Through  2D State Space")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')

%multiple paths
figure(3)
clf

velocities = linspace(initialVelocity, 0, 100);
a1 = 2 * [0.5 0.25 -0.21 0.061];
omega1 = 1/20;
r1 = randomSignal(velocities, uMin, uMax, a1, omega1);
a2 = 2 * [0.1 -0.8 -0.56 0.3 -0.15];
omega2 = 1/30;
r2 = randomSignal(velocities, uMin, uMax, a2, omega2);
a3 = [0.5 0.25];
omega3 = 1/10;
r3 = -0.0018 + randomSignal(velocities, uMin, uMax, a3, omega3);

hold on
plot(velocities, r1)
plot(velocities, r2)
plot(velocities, r3)
yline(uMax)
yline(uMin)
hold off

figure(4)
clf
hold on

altitudes = integralFromula(velocities, initial, @(x) uMin, airDensity, mass);
plot(velocities, altitudes, '--k')

altitudes = integralFromula(velocities, initial, @(x) uMax, airDensity, mass);
plot(velocities, altitudes, '-.k')

altitudes = integralFromula(velocities, initial, @(x) randomSignal(x, uMin, uMax, a1, omega1), airDensity, mass);
plot(velocities, altitudes, 'k')

altitudes = integralFromula(velocities, initial, @(x) randomSignal(x, uMin, uMax, a2, omega2), airDensity, mass);
plot(velocities, altitudes, 'k')

altitudes = integralFromula(velocities, initial, @(x) -0.0018 + randomSignal(x, uMin, uMax, a3, omega3), airDensity, mass);
plot(velocities, altitudes, 'k')

hold off
ylim([0 1000])
xlim([0 150])
formatIEEE();
title("Flight Paths for Various Deployment Sequences")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Fully Retracted", "Fully Deployed", "Non-Fixed Sequences", 'Location', 'southwest', 'FontSize', 8)


%future cone
figure(5)
clf
point = [80 400];
velocities = linspace(0, point(1), 100);
kMax = kValue(airDensity, mass, uMax);
minAltitudes = altitudeVelocity(velocities, point, k);
maxAltitudes = altitudeVelocity(velocities, point, kMax);

hold on
plot(velocities, minAltitudes, '--k')
plot(velocities, maxAltitudes, '-.k')
scatter(point(1), point(2), 'k', 'filled')
text(point(1), point(2), " Initial State", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'top')
hold off

ylim([0 1000])
xlim([0 150])
formatIEEE();
title("Example Future Cone")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Upper Bound F^+", "LowerBound F^-", 'Location', 'northeast', 'FontSize', 8)

%past cone
figure(6)
clf

hold on
plot(velocities, maxAltitudes, '-.k')
plot(velocities, minAltitudes, '--k')
scatter(point(1), point(2), 'k', 'filled')
text(point(1), point(2), " Initial State", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'bottom')
hold off

velocities = linspace(point(1), maxVelocity, 100);
kMax = kValue(airDensity, mass, uMax);
minAltitudes = altitudeVelocity(velocities, point, k);
maxAltitudes = altitudeVelocity(velocities, point, kMax);

hold on
plot(velocities, maxAltitudes, '--k')
plot(velocities, minAltitudes, '-.k')
hold off

ylim([0 1000])
xlim([0 150])
formatIEEE();
title("An Example Point's Past and Future Cones")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Lower Bound (F^- or P^-)", "Upper Bound (F^+ or P^+)", 'Location', 'northeast', 'FontSize', 8)

%Target's past cone
figure(7)
clf
target = 750;
velocities = linspace(0, maxVelocity, 100);
upperBound = altitudeVelocity(velocities, [0 target], kMax);
lowerBound = altitudeVelocity(velocities, [0 target], k);

hold on
plot(velocities, upperBound, 'k');
plot(velocities, lowerBound, 'k');
scatter(0, target, 'k', 'filled')
text(0, target, " Target Apogee", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'bottom')
hold off

ylim([0 1000])
xlim([0 150])
formatIEEE();
title("The Past Cone of the Target Apogee")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')

%Apogee Centric Figures
figure(8)
clf
step2Velocity = 130;
step3Velocity = 110;
step4Velocity = 90;
step1Velocities = linspace(initialVelocity, step2Velocity, 100);
step1Altitudes = altitudeVelocity(step1Velocities, initial, k);
step1PredictionVelocities = linspace(step2Velocity, 0, 100);
step2 = [step2Velocity step1Altitudes(100)];
step1PredictionAltitudes = altitudeVelocity(step1PredictionVelocities, step2, k);
step2Velocities = linspace(step2Velocity, step3Velocity);
k2 = 1.75 * k;
step2Altitudes = altitudeVelocity(step2Velocities, step2, k2);
step2PredictionVelocities = linspace(step3Velocity, 0, 100);
step3 = [step3Velocity step2Altitudes(100)];
step2PredictionAltitudes = altitudeVelocity(step2PredictionVelocities, step3, k2);
step3Velocities = linspace(step3Velocity, step4Velocity);
k3 = 2.5 * k;
step3Altitudes = altitudeVelocity(step3Velocities, step3, k3);
step3PredictionVelocities = linspace(step4Velocity, 0, 100);
step4 = [step4Velocity step3Altitudes(100)];
step3PredictionAltitudes = altitudeVelocity(step3PredictionVelocities, step4, k3);
k4 = 3 * k;
finalVelocities = linspace(step4Velocity, 0, 100);
finalAltitudes = altitudeVelocity(finalVelocities, step4, k4);



hold on
plot(step1Velocities, step1Altitudes, 'k')
plot(step1PredictionVelocities, step1PredictionAltitudes, ':k')
scatter(initial(1), initial(2), 'k', 'filled')
text(initial(1), initial(2), "Step 1 ", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'right', 'VerticalAlignment', 'top')
scatter(step2(1), step2(2), 'k', 'filled')
text(step2(1), step2(2), "Step 2 ", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'right', 'VerticalAlignment', 'top')
plot(step2Velocities, step2Altitudes, 'k')
plot(step2PredictionVelocities, step2PredictionAltitudes, ':k')
scatter(step3(1), step3(2), 'k', 'filled')
text(step3(1), step3(2), "Step 3 ", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'right', 'VerticalAlignment', 'top')
plot(step3Velocities, step3Altitudes, 'k')
plot(step3PredictionVelocities, step3PredictionAltitudes, ':k')
scatter(step4(1), step4(2), 'k', 'filled')
text(step4(1), step4(2), "Step 4 ", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'right', 'VerticalAlignment', 'top')
plot(finalVelocities, finalAltitudes, 'k')
scatter(0, finalAltitudes(100), 'k', 'filled')
text(0, finalAltitudes(100) - 10, " Target", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'top')
hold off

ylim([0 1000])
xlim([0 150])
formatIEEE();
title("Apogee-Centric Control Architecture")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Flight Path", "Intermidiate Predictions", 'Location', 'northeast', 'FontSize', 8)

%flight planning explaination
figure(9)
clf
intersectionVelocity = 80;
planVelocities = linspace(maxVelocity, intersectionVelocity, 100);
plan = (altitudeVelocity(planVelocities, [0 target], k) + altitudeVelocity(planVelocities, [0 target], kMax))/2;
convVelocities = linspace(0, intersectionVelocity, 100);
followedPlan = (altitudeVelocity(convVelocities, [0 target], k) + altitudeVelocity(convVelocities, [0 target], kMax))/2;
navVelocities = linspace(initialVelocity, intersectionVelocity, 100);
navPath = altitudeVelocity(navVelocities, [intersectionVelocity plan(100)], kMax);

hold on
plot(planVelocities, plan, '--k')
plot(navVelocities, navPath, 'k')
plot(convVelocities, followedPlan, 'k')
scatter(0, target, 'k', 'filled')
text(0, target, " Target", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'bottom')
scatter(initialVelocity, navPath(1), 'k', 'filled')
text(initialVelocity, navPath(1), " Burnout", 'FontName', 'Times New Roman', 'FontSize', 8, 'HorizontalAlignment', 'left', 'VerticalAlignment', 'bottom')
hold off

ylim([0 1000])
xlim([0 150])
formatIEEE();
title("Flight Planning Control Architecture")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Flight Plan", "Flight Path", 'Location', 'northeast', 'FontSize', 8)

%derivative followability
figure(10)
clf


InitialVelocity = 150; %m/s
InitialAltitude = 100; %m
Mass = 6; %
AngleSamples = 500;
LaunchSitePressure = 101325; %Pa
LaunchSiteTemperature = 288.15; %K
MinimumDragArea = 0.0025; %m^2
MaximumDragArea = 0.01; %m^2

[~, x] = ODETimeSolution(initialVelocity, InitialAltitude, 0.9 * pi/2, LaunchSiteTemperature, LaunchSitePressure, MinimumDragArea, Mass);
[~, x1] = ODETimeSolution(initialVelocity, InitialAltitude, 0.9 * pi/2, LaunchSiteTemperature, LaunchSitePressure, MaximumDragArea, Mass);
[~, x2] = ODETimeSolution(initialVelocity, InitialAltitude, 0.9 * pi/2, LaunchSiteTemperature, LaunchSitePressure, MaximumDragArea * 0.7, Mass);

[x, y] = makeStateFormat(x);
[x1, y1] = makeStateFormat(x1);
[x2, y2] = makeStateFormat(x2);

[~, x3] = ODETimeSolution(x1(30,1), y1(30), x1(30,2), LaunchSiteTemperature, LaunchSitePressure, MinimumDragArea, Mass);
[x3, y3] = makeStateFormat(x3);


figure(11)
clf
hold on
plot3(x(:,1), x(:,2), y)
plot3(x1(:,1), x1(:,2), y1)
plot3(x2(:,1), x2(:,2), y2)
plot3(x3(:,1), x3(:,2), y3)

plot(x3(:,1), x3(:,2))
plot(x2(:,1), x2(:,2))
hold off

formatIEEE();
title("Flight Planning Control Architecture")
xlabel("Velocity (m/s)", 'FontSize', 9, 'FontName', 'Times New Roman')
ylabel("Altitude (m)", 'FontSize', 9, 'FontName', 'Times New Roman')
legend("Flight Plan", "Flight Path", 'Location', 'northeast', 'FontSize', 8)