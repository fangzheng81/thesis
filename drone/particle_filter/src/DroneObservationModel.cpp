#include "particle_filter/DroneObservationModel.h"
#include "particle_filter/MapModel.h"

DroneObservationModel::DroneObservationModel(ros::NodeHandle* nh, std::shared_ptr<MapModel> _mapModel)
  : libPF::ObservationModel<DroneState>()
{
  _map = _mapModel->getMap();
  _baseToSensorTransform.setIdentity();
  nh->param<double>("/laser_z_hit", _ZHit, 0.5);
  nh->param<double>("/laser_z_short", _ZShort, 0.05);
  nh->param<double>("/laser_z_rand", _ZRand, 0.5);
  nh->param<double>("/laser_z_max", _ZMax, 0.05);
  nh->param<double>("/laser_sigma_hit", _SigmaHit, 0.2);
  nh->param<double>("/laser_lambda_short", _LambdaShort, 0.1);
  nh->param<double>("/min_range", _minRange, 0.01);
  nh->param<double>("/max_range", _maxRange, 14);

  ROS_INFO("Drone observation model has been created!\n");
}

DroneObservationModel::~DroneObservationModel()
{
}

double DroneObservationModel::measure(const DroneState& state) const
{
  // transform current particle's pose to its sensor frame
  tf2::Transform particlePose;
  geometry_msgs::Transform particlePose_g;
  particlePose_g.translation.x = state.getXPos();
  particlePose_g.translation.y = state.getYPos();
  particlePose_g.translation.z = state.getZPos();

  tf::Quaternion particlePoseOrientation;
  particlePoseOrientation = tf::createQuaternionFromRPY(state.getRoll(), state.getPitch(), state.getPitch());
  particlePoseOrientation.normalize();
  // Convert tf::quaternion to std_msgs::quaternion to be accepted in the odom msg
  tf::quaternionTFToMsg(particlePoseOrientation, particlePose_g.rotation);

  tf2::fromMsg(particlePose_g, particlePose);

  tf2::Transform globalLaserOriginTf = particlePose * _baseToSensorTransform;

  // Raycasting Origin Point
  octomap::point3d originP(globalLaserOriginTf.getOrigin().getX(), globalLaserOriginTf.getOrigin().getY(),
                           globalLaserOriginTf.getOrigin().getZ());

  // Transform Pointcloud
  Eigen::Matrix4d globalLaserOrigin;

  geometry_msgs::Transform transformMsg;
  transformMsg = tf2::toMsg(globalLaserOriginTf);
  Eigen::Affine3d tmp = tf2::transformToEigen(transformMsg);

  pcl::PointCloud<pcl::PointXYZ> pcTransformed;
  pcl::transformPointCloud(_observedMeasurement, pcTransformed, tmp);

  pcl::PointCloud<pcl::PointXYZ>::const_iterator pcIter = pcTransformed.begin();
  std::vector<float>::const_iterator rangesIter = _observedRanges.begin();

  double weight = 1.0;

  //#pragma omp parallel  for
  for (; pcIter != pcTransformed.end(); pcIter++, rangesIter++)
  {
    // Probability for weight
    double p = 0.0;

    float obsRange = *rangesIter;
    float raycastRange;
    octomap::point3d direction(pcIter->x, pcIter->y, pcIter->z);
    direction = direction - originP;

    octomap::point3d end;

    octomap::ColorOcTreeNode* colorNode;
    if (_map->castRay(originP, direction, end, true, 1.5 * _maxRange))
    {
      ROS_ASSERT(_map->isNodeOccupied(_map->search(end)));
      colorNode = _map->search(end);
      raycastRange = (originP - end).norm();
    }

    // Particle in occupied space(??)
    if (raycastRange == 0)
      continue;

    float z = obsRange - raycastRange;

    // todo check normalization factors in Probabilistics Robotics page 138
    if (obsRange < _maxRange)
      p += _ZHit * 1 / (std::sqrt(2 * M_PI * _SigmaHit * _SigmaHit)) * exp(-(z * z) / (2 * _SigmaHit * _SigmaHit));

    if (z < 0)
      p += _ZShort * _LambdaShort * exp(-_LambdaShort * obsRange);

    if (obsRange >= _maxRange)
      p += _ZMax * 1.0;

    if (obsRange < _maxRange)
      p += _ZRand * 1.0 / _maxRange;

    ROS_ASSERT(p > 0.0);
    weight *= p;
  }

  return weight;
}

void DroneObservationModel::setMap(const std::shared_ptr<octomap::ColorOcTree>& map)
{
  _map = map;
}

void DroneObservationModel::setBaseToSensorTransform(const tf2::Transform& baseToSensorTF)
{
  _baseToSensorTransform = baseToSensorTF;
}

void DroneObservationModel::setObservedRanges(const pcl::PointCloud<pcl::PointXYZ>& observed,
                                              const std::vector<float>& ranges)
{
  _observedMeasurement = observed;
  _observedRanges = ranges;
}