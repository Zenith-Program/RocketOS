function dragArea = getDragArea(acceleration, velocity, altitude, angle, mass, p0, t0)
    g = 9.8;
    dragArea = -2 * mass * sin(angle) / (densityFromAltitude(altitude, p0, t0) * velocity * velocity) * (acceleration + g);
end

function density = densityFromAltitude(altitude, p0, T0)
    % Constants
    L = 0.0065;      % K/m temperature lapse rate
    g = 9.80665;     % m/s^2 gravitational constant
    R = 8.31446;     % J/(mol*K) Ideal gas constant
    M = 0.0289652;   % kg/mol Molar mass of air
    density = p0*M/(R*T0)*(1-L*altitude/T0).^(g*M/(R*L)-1);
end

LaunchSitePressure = 101325; %Pa
LaunchSiteTemperature = 288.15; %K

getDragArea(-15, 20, 700, pi/4, 6, LaunchSitePressure, LaunchSiteTemperature)