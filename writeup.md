## Writeup

---

**MPC Project**

The goals / steps of this project are the following:
* Implement Model Predicted Controller in C++.
* Tune timestep length, elapsed duration and cost fuction to go around the lake in project simulator as fast as possible.
* Write how I tune each parameter.

#### [Rubric](https://review.udacity.com/#!/rubrics/896/view)

### Here I will consider the rubric points individually and describe how I addressed each point in my implementation.

---

### Discussion
I implemented the model predicted controller using the udacity provided template. Some variables are added so that I can see the detail of the cost function.

#### 1. Here I briefly discuss how each parameter affect the motion of the car in simulation.
There are 10 parameters I tuned.
First of all, I explain the influence of timestep length N, elapsed duration and target speed(MPC.cpp line 9-10 and main.cpp line 91-92). Timestep length is the value how important you think the future value is. If it is too low, you only take into acount the small time in the future. So it cannot deal with the sharp curve. On the other hand, if it is too high, you takes into account the long time. Normally, it looks good things. However, when the timestep length is too much, it is not so precise (because there are some unmodeled dynamics) that the prediction result is no longer reliable. Also, elapsed duration influences the model accuracy and computation time. If it is too low, the computation time increses and model can only take into account. On the other hand, if it is too high, the model is no longer precise because of finite difference approximation. Finally, target speed means as it is.
Next, I explain cost function. Cost function has three parts. First, cross track error , direction error and the speed error penalize the position, direction and speed error of car. Second, the costs of steering and throttle penalize the amount of control. It is good for energy consumption. Third, the change of steering and throttle weight penalize the rapid change of control. It makes controller more stable.
By tuning all the parmter I mentioned, my controller makes the car go around much faster than PID control project.

#### 2. Here I briefly discuss how I deal with the latency.
I deal with latency in main.cpp 141-150 lines. First, I get current state. Next, I predict the state 0.1s later from the current state. 0.1s is the time where the control is executed. Then, I solve the optimization problem. So since the model takes into accout the latency, its result becomes well.

#### 3.  Here I briefly discuss how I tune each parameter.
This is the final parameters I have tuned.

| Parameters    | Values    |
|:-------------:|:-------------:|
| Timestep length N                 |  14       |
| Elapsed duration dt               |  0.1      |
| Target speed                      |  90.0     |
| CTE weight                        |  1300     |
| Direction error weight            |  45000    |
| Speed error weight                |  1        |
| Steering control weight           |  1000     |
| Throttle control weight           |  1        |
| Change of steering control weight |  10000000 |
| Change of steering control weight |  1000     |

Here is a link to [my video result](./videos/MPC.mp4).

I manually tune my parameters.
First, elapsed duration is the timing where the control is executed, so I chose 0.1.
Next, I tuned weights of the cost function. I print the present state and each cost so that I can find which cost function influences prediction. For examle, if small number of the costs are relatively bigger, it is bad tuning because small number of costs determine the control. Using those information, I could tune weight of the cost function.
Finally, I tuned timestep length so the controller can deal with the sharp curve of track. 

#### 4. Here I briefly discuss how I improve my program.

There are some ways to improve my program. I can weight the CTE and epsi more if they  are nearer the present. This is because the control and state become less certain if those are far from the present. So the nearer the control and state are, the more they influence the prediction. Also, my paramters were tuned so that the car can go around the track as fast as possible. Therefore, the car's speed is often changed because the throttle control weight and the change of throttle control weight are small. I can tune those parameter so that the speed and energy cosumption are balanced 
