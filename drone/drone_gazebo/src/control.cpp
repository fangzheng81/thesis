/* Implementation of the Controller methods */

#include "drone_gazebo/control.h"

namespace control
{
/******************************/
/*        Constructor         */
/******************************/

Controller::Controller()
{
  ROS_INFO("Controller empty object created");
}

/******************************/
/* Constructor with arguments */
/******************************/

Controller::Controller(char* argv[])
{
  // Make the desired height a parameter
  setDesiredHeight(atoi(argv[1]));

  ros::Rate loop_rate(10);

  // Initialize the Subscriber
  _heightListener = _nh.subscribe("/height", 1, &Controller::callback, this);

  // Initialize the Publisher
  _cmdvelPublisher = _nh.advertise<geometry_msgs::Twist>("/cmd_vel", 1);
}

/******************************/
/*        Destructor          */
/******************************/

Controller::~Controller()
{
  ROS_INFO("Class Controller has been destroyed\n");
}

/******************************/
/*         Callback           */
/******************************/

void Controller::callback(const std_msgs::Float64::ConstPtr& msg)
{
  float currentHeight = msg->data;
  geometry_msgs::Twist velMsg;

  _cmdvelPublisher.publish(velMsg);

  float z;  // distance to cover
  if (currentHeight != getDesiredHeight())
  {
    // z = v * t; // t: sec, z: m, v = m/s
    z = getDesiredHeight() - currentHeight;
    z = roundf(z * 1000) / 1000;  // round to nearest
    ROS_INFO("Height to cover: %f\n", z);
    velMsg.linear.z = z * 100 / 30;  // Convert z to cm and divide with time
    ROS_INFO("Velocity to publish: %f\n", velMsg.linear.z);
    _cmdvelPublisher.publish(velMsg);
  }
  else
  {
    velMsg.linear.z = 0;
    ROS_INFO("Hover mode\n");
  }
}

}  // namespace control`