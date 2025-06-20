clc
clear
close all

%% Mechanical Parameters
m1 = 0.1975; % motor stator pendulum arm mass
m2 = 0.2545; % motor rotor and reaction mass kg
L1 = 0.11; % Pendulum Arm center of mass distance to axis
L2 = 0.176; % Motor center of mass distance to axis
g = 9.81; % Gravitational acceleration
R = 0.0655;

I1 = m1 * L1^2 / 12; % pendulum inertia with respect to CoG
I2 = m2 * R^2 / 2; % wheel inertia with respect to CoG

b_b = 0.01; %Pendulum Arm axis viscous friction посмотреть в инвенторе
%% Motor Parameters
Vmax = 16;
Ke = 0.29; %0.68;% Motor EMF constant
Kt = 0.27;%0.98;% %Motor Torque constant

nom_torque = 0.21;%0.21;
nom_vel = 35.9;
b_R = nom_torque/nom_vel;%Motor viscous friction coefficient !!!!! отношение номинального момента к номинальной угловой скорости????

Rm = 10.9; % Motor coil resistance
Lm = 0.01;

%% matrices

a = m1 * L1^2 + m2 * L2^2 + I1;
b = (m1*L1 + m2*L2)*g;
c = -((a + I2)/(a*I2)) * (Kt/Rm);

a21 = b/a;
a24 = Kt*Ke/(a*Rm);
a41 = -b/a;
a44 = c * Ke;
b2 = -Kt/(a*Rm);
b4 = c;


A = [0   1   0   0  ;
    a21  0   0   a24;
    0    0   0   1  ;
    a41  0   0   a44];

B = [0;
    b2;
    0;
    b4];

%% Output

C = [1 0 0 0;    %pend angle
     0 1 0 0;    %pend angular vel
     0 0 1 0;    % wheel angle
     0 0 0 1];   % wheel speed 

D = [0;0;0;0];

%% Build system
%sys = ss(A, B, C, D)
%eig(A);
x0 = [3; 0; 0; 0]; %pend angle, pend angular vel, wheel speed

%% Controller
Ts = 0.01;
Q = [1.5e2 0   0 0;
     0   2.852e2 0 0;
     0   0   1 0;
     0   0   0 10];
Re =28;
K = lqrd(A, B, Q, Re, Ts)




