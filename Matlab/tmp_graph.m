% t = 0:100;
% amplitude = 2;
% period = 11;
% r = mod(t,period);
% if (r>=0) && (r<period/4)
%     y = 4*amplitude*t/period;
% elseif (r>=period/4) && (r<3*period/4)
%     y = 2*amplitude - 4*amplitude*t/period;
% else
%     y = -4*amplitude + 4*amplitude*t/period;
% end
% plot(t,y)
% y = [];
% 
% for t = 0:1000
%     r = mod(t,period);
%     if (r>=0) && (r<period/4)
%         y(end+1) = 4*amplitude*t/period;
%     elseif (r>=period/4) && (r<3*period/4)
%         y(end+1) = 2*amplitude - 4*amplitude*t/period;
%     else
%         y(end+1) = -4*amplitude + 4*amplitude*t/period;
%     end
% end


        
     


var = (t-period/4);
r = mod((mod(var, period)+period),period);
f = (4*amplitude/period)*abs(r - period/2) - amplitude;
y = (2*amplitude/pi)*asin(sin((2*pi/period)*t));
x = amplitude - (2*amplitude/pi)*acos(cos((2*pi/period)*t));
plot(t, y, 'blue', t, f, 'red', t, x, 'green');
legend('asin', 'new', 'acos');



% 
% for i = 1 : 100
%     x(i) = i; 
% end
% tri = 0;
% y =  zeros(1, 100);
% amplitude = 2;
% period = 19;
% 
% for t = 1:100
%     r = mod(t,period);
%     if (r>=0) && (r<period/2)
%         tri = 2*amplitude*t/period;
%     else
%         tri = amplitude*(1-2*t/period);
%     end
%     y(t) = tri;
% end
% 
% plot(x, y);


% 
% function tri = tmp_graph(t)
%     amplitude = 2;
%     period = 19;
%     r = mod(t,period);
%     if (r>=0) && (r<period/2)
%         tri = 2*amplitude*t/period;
%     else
%         tri = amplitude*(1-2*t/period);
%     end
% end








