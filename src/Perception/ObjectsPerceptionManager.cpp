#include "overworld/Perception/ObjectsPerceptionManager.h"

#define TO_HALF_RAD M_PI / 180. / 2.

namespace owds {

void ObjectsPerceptionManager::getPercepts(const std::map<std::string, Object>& percepts)
{
  for(auto& percept : percepts)
    {
        auto it = entities_.find(percept.second.id());
        if(it == entities_.end())
        {
            if(percept.second.isLocated() == false)
              continue;

            it = entities_.insert(std::pair<std::string, Object>(percept.second.id(), percept.second)).first;
            addToBullet(it->second);
        }
        
        if(it->second.isInHand())
        {
          if(lost_objects_nb_frames_.find(percept.first) != lost_objects_nb_frames_.end())
            if(lost_objects_nb_frames_[percept.first] < 5)
            {
              if(it->second.pose().distanceSqTo(percept.second.pose()) > 0.55) // Add a condition on time ?
                it->second.removeFromHand();
              else
                updateEntityPose(it->second, it->second.getHandIn()->pose(), ros::Time::now());
            }
        }
        else if (percept.second.hasBeenSeen())
          updateEntityPose(it->second, percept.second.pose(), percept.second.lastStamp());
    }
}

void ObjectsPerceptionManager::reasoningOnUpdate()
{
  std::vector<Object*> no_data_objects;

  for(auto& object : entities_)
  {
    if(object.second.isStatic() == true)
      continue;

    if(object.second.isInHand() == false)
    {
      if(object.second.getNbFrameUnseen() >= 5)
      {
        auto pois_in_fov = getPoisInFov(object.second);
        if(pois_in_fov.size() != 0)
        {
          if(shouldBeSeen(object.second, pois_in_fov))
          {
            auto it_unseen = lost_objects_nb_frames_.find(object.first);
            if(it_unseen == lost_objects_nb_frames_.end())
              it_unseen = lost_objects_nb_frames_.insert(std::make_pair<std::string, size_t>(object.second.id(), size_t(0))).first;
            it_unseen->second++;
            if(it_unseen->second > 5)
              removeEntityPose(object.second);
          }
        }
      }
    }
  }
}

std::vector<PointOfInterest> ObjectsPerceptionManager::getPoisInFov(const Object& object)
{
  if(myself_agent_ == nullptr)
  {
    ShellDisplay::error("[ObjectsPerceptionManager] has no agent defined");
    return object.getPointsOfInterest();
  }
  if(myself_agent_->getHead() == nullptr)
    return object.getPointsOfInterest();

  std::vector<PointOfInterest> pois_in_fov;

  for(const auto& poi : object.getPointsOfInterest())
  {
    bool poi_is_valid = true;
    for(const auto& point : poi.getPoints())
    {
      auto poi_in_map = point.transformIn(object.pose());
      auto poi_in_head = poi_in_map.transformIn(myself_agent_->getHead()->pose());
      if (myself_agent_->getFieldOfView().hasIn(poi_in_head))
        continue;
      else
      {
        poi_is_valid = false;
        break;
      }
    }

    if(poi_is_valid)
      pois_in_fov.push_back(poi);
  }

  return pois_in_fov;
}

bool ObjectsPerceptionManager::shouldBeSeen(const Object& object, const std::vector<PointOfInterest>& pois)
{
  if(myself_agent_->getHead() == nullptr)
    return false;

  for(const auto& poi : pois)
  {
    std::vector<std::array<double, 3>> from_poses(poi.getPoints().size(), myself_agent_->getHead()->pose().arrays().first);
    std::vector<std::array<double, 3>> to_poses;

    for(const auto& point : poi.getPoints())
      to_poses.push_back(point.arrays().first);
    auto ray_cast_info = bullet_client_->rayTestBatch(from_poses, to_poses, poi.getPoints().size(), true);

    if(ray_cast_info.size() == 0)
      return true;
    else
    {
      for(auto& info : ray_cast_info)
      {
        bullet_client_->addUserDebugLine({info.m_hitPositionWorld[0],
                                          info.m_hitPositionWorld[1],
                                          info.m_hitPositionWorld[2]},
                                         myself_agent_->getHead()->pose().arrays().first,
                                         {1,0,0}, 1, 2.0);
        
      }
    }
  }

  return false;
}

} // namespace owds