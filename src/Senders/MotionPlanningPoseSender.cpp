#include "overworld/Senders/MotionPlanningPoseSender.h"

#include <geometry_msgs/PoseStamped.h>

namespace owds {

MotionPlanningPoseSender::MotionPlanningPoseSender(ros::NodeHandle* nh_, ObjectsPerceptionManager& object_perception_manager)
    : object_perception_manager_(object_perception_manager)
{
    get_pose_service_ = nh_->advertiseService("/overworld/getPose", &MotionPlanningPoseSender::onGetPoseService, this);  //TODO: Change for a private node handle
}

bool MotionPlanningPoseSender::onGetPoseService(pr2_motion_tasks_msgs::GetPose::Request& req, pr2_motion_tasks_msgs::GetPose::Response& res)
{
    const auto objects = object_perception_manager_.getEntities();
    for (const std::string& id : req.ids)
    {
        geometry_msgs::PoseStamped pose;
        if (objects.find(id) == objects.end())
        {
            pose.header.frame_id = "";
        }
        else
        {
            const Object* obj = objects.at(id);
            if (obj->isLocated() == false)
            {
                pose.header.frame_id = "";
            }
            else
            {
                pose.pose = obj->pose().toPoseMsg();
                pose.header.stamp = obj->lastStamp();
                pose.header.frame_id = "world";
            }
        }
        res.poses.push_back(pose);
    }
    return true;
}

} // namespace owds
