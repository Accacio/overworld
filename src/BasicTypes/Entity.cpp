#include "overworld/BasicTypes/Entity.h"

#include <ros/ros.h>

namespace owds {
    
Entity::Entity(const std::string& id, bool is_true_id): id_(id), 
                                                        is_true_id_(is_true_id),
                                                        is_located_(false),
                                                        bullet_id_(-1)
{}

void Entity::updatePose(const Pose& pose, ros::Time stamp)
{
    pose_ = pose;
    is_located_ = true;
    last_pose_ = stamp;
}

void Entity::updatePose(const std::array<double, 3>& translation, const std::array<double, 4>& rotation)
{
    updatePose(translation, rotation, ros::Time::now());
}

void Entity::updatePose(const std::array<double, 3>& translation, const std::array<double, 4>& rotation, ros::Time stamp)
{
    pose_ = Pose(translation, rotation);
    is_located_ = true;
    last_pose_ = stamp;
}

void Entity::updatePose(const geometry_msgs::PoseStamped& pose)
{
    pose_ = Pose(pose);
    is_located_ = true;
    last_pose_ = pose.header.stamp;
}

const Pose& Entity::pose() const
{
    if (!is_located_){
        throw UnlocatedEntityError(id_);
    }
    return pose_;
}

void Entity::setId(const std::string& id, bool is_true_id)
{
    id_ = id;
    is_true_id_ = is_true_id;
}

geometry_msgs::TransformStamped Entity::toTfTransform() const
{
    if (!isLocated())
    {
        throw std::runtime_error("Called toTfTransform on a non located entity: '" + id_ + "'.");
    }
    geometry_msgs::TransformStamped transform;
    transform.header.stamp = last_pose_;
    transform.header.frame_id = "map";
    transform.child_frame_id = id_;
    transform.transform = pose_.toTransformMsg();
    return transform;
}

visualization_msgs::Marker Entity::toMarker(int id, double lifetime, const std::string& ns) const
{
    if (!isLocated())
    {
        throw std::runtime_error("Called toMarker on a non located entity: '" + id_ + "'.");
    }
    visualization_msgs::Marker marker;
    marker.header.frame_id = "map";
    marker.header.stamp = last_pose_;
    marker.id = id;
    marker.lifetime = ros::Duration(lifetime);
    switch (shape_.type)
    {
    case ShapeType_e::SHAPE_MESH:
        marker.type = marker.MESH_RESOURCE;
        marker.mesh_resource = shape_.mesh_resource;
        marker.mesh_use_embedded_materials = true;
        break;

    case ShapeType_e::SHAPE_SPEHERE:
        marker.type = marker.SPHERE;
        break;
    case ShapeType_e::SHAPE_CUBE:
        marker.type = marker.CUBE;
        break;
    case ShapeType_e::SHAPE_CYLINDER:
        marker.type = marker.CYLINDER;
        break;
    default:
        throw std::runtime_error("toMarker has been called on entity '" + id_ + "' + but its ShapeType is not defined.");
        break;
    }
    marker.scale.x = shape_.scale[0];
    marker.scale.y = shape_.scale[1];
    marker.scale.z = shape_.scale[2];
    marker.color.r = shape_.color[0];
    marker.color.g = shape_.color[1];
    marker.color.b = shape_.color[2];
    marker.color.a = 1.0;
    marker.ns = ns;
    marker.action = marker.ADD;
    marker.pose = pose_.toPoseMsg();
    return marker;
}

} // namespace owds
