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
    h;
end