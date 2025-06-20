clc
clear
close all

Ts = 0.01; % timestep

%% Mechanical Parameters
m1 = 0.1975; % motor stator pendulum arm mass
m2 = 0.2545; % motor rotor and reaction mass kg
L1 = 0.06;   % pendulum Arm center of mass distance to axis
L2 = 0.088;  % wheel center of mass distance to axis
g = 9.81;    % gravitational acceleration
R = 0.0655;  % wheel radius

I1 = m1 * L1^2 / 12; % pendulum inertia with respect to CoG
I2 = m2 * R^2 / 2; % wheel inertia with respect to CoG

b_p = 0.007; % pendulum arm axis viscous friction посмотреть в инвенторе
b_r = 0;

%% Motor Parameters
Vmax = 16;
Ke = 0.29; % Motor EMF constant
Kt = 0.27; % Motor Torque constant

nom_torque = 0.21;
nom_vel = 35.9;

Rm = 22;   % motor coil resistance
Lm = 0.01; % motor induction

%% Create State Space Matrices
% State transition matrix A and control vector B

a = m1 * L1^2 + m2 * L2^2 + I1;
b = (m1*L1 + m2*L2)*g;

a21 = b/a;
a22 = -b_p/a;
a24 = (Rm*b_r + Kt*Ke)/(a*Rm);

a41 = -b/a;
a42 = b_p/a;
a44 = -(a + I2)*(b_r + Kt*Ke/Rm)/(I2 * a);

b2 = -Kt/(a*Rm);
b4 = -((a + I2)/(a*I2)) * (Kt/Rm);


A = [0   1     0   0  ;
    a21  a22   0   a24;
    0    0     0   1  ;
    a41  a42   0   a44];

B = [0;
    b2;
    0;
    b4];

% Output

C = [1 0 0 0;    %pend angle
     0 1 0 0;    %pend angular vel
     0 0 1 0;    % wheel angle
     0 0 0 1];   % wheel speed 

D = [0;0;0;0];

%% Build system
% creating continuous the state space model
sys = ss(A, B, C, D);

% convert to discrete form
sysd = c2d(sys, Ts);
Ad = sysd.A;
Bd = sysd.B;

%% LQR Controller

Qe = [1 0 0 0;
     0 1 0 0;
     0 0 1 0;
     0 0 0 1];
Re = 1;

[K,S,e] = dlqr(Ad, Bd, Qe, Re);
disp('K gain matrix:');
disp(K);


x0 = [0.2; 0; 0; 0]; % pend angle, pend angular vel, wheel speed

%% Stability Analysis
% The system is said to be asymptotically stable if all eigenvalues of the closed-loop matrix A_cl = A - BK
% lie strictly inside the unit circle in the complex plane

eigvals = eig(Ad - Bd * K);
disp('Eigenvalues of closed-loop system:');
disp(eigvals);

if all(abs(eigvals) < 1)
    disp('System is asymptotically stable');
else
    disp('System is unstable');
end

%% Simulation

% todo
