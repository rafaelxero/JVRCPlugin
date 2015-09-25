/**
   @author Shin'ichiro Nakaoka
*/

#include "JVRCTaskInfo.h"
#include <cnoid/YAMLReader>
#include <boost/format.hpp>

using namespace std;
using namespace cnoid;


JVRCEvent::JVRCEvent(const std::string& type, JVRCTask* task)
    : type_(type),
      label_(type),
      task_(task)
{
    level_ = 0;
}


JVRCEvent::JVRCEvent(const std::string& type, JVRCTask* task, Mapping* info)
    : type_(type),
      label_(type),
      task_(task)
{
    string label;
    if(info->read("label", label)){
        label_ = label;
    }

    level_ = 0;
    int level;
    if(info->read("level", level)){
        level_ = level;
    }
}


JVRCEvent::JVRCEvent(const JVRCEvent& org)
    : type_(org.type_),
      label_(org.label_),
      level_(org.level_),
      automaticRecordTime_(org.automaticRecordTime_),
      manualRecordTime_(org.manualRecordTime_),
      task_(org.task_)
{

}


void JVRCEvent::setLabel(const std::string& label)
{
    label_ = label;
}


JVRCEvent* JVRCEvent::clone()
{
    return new JVRCEvent(*this);
}


bool JVRCEvent::isSameAs(JVRCEvent* event)
{
    return (type_ == event->type_ && label_ == event->label_ && task_ == event->task_);
}


double JVRCEvent::time() const
{
    if(manualRecordTime_){
        return *manualRecordTime_;
    }
    if(automaticRecordTime_){
        return *automaticRecordTime_;
    }
    return 0.0;
}


JVRCGateEvent::JVRCGateEvent(JVRCTask* task, Mapping* info)
    : JVRCEvent("gate", task, info)
{
    index_ = 0;

    isLabelSpecified = info->find("label")->isValid();
    
    locations[0].setZero();
    locations[1].setZero();

    const Listing& location = *info->findListing("location");
    if(location.isValid()){
        if(location.size() == 2){
            for(int i=0; i < 2; ++i){
                const Listing& p = *location[i].toListing();
                if(p.size() == 2){
                    locations[i] << p[0].toDouble(), p[1].toDouble(), 0.0;
                }
            }
        }
    }
}


JVRCGateEvent::JVRCGateEvent(const JVRCGateEvent& org)
    : JVRCEvent(org)
{
    index_ = org.index_;
    locations[0] = org.locations[0];
    locations[1] = org.locations[1];
}


JVRCEvent* JVRCGateEvent::clone()
{
    return new JVRCGateEvent(*this);
}


bool JVRCGateEvent::isSameAs(JVRCEvent* event)
{
    JVRCGateEvent* gate = dynamic_cast<JVRCGateEvent*>(event);
    if(gate){
        if(index_ == gate->index_){
            return JVRCEvent::isSameAs(event);
        }
    }
    return false;
}


void JVRCGateEvent::setIndex(int i)
{
    index_ = i;

    if(!isLabelSpecified){
        setLabel(str(boost::format("Gate %1%") % (i + 1)));
    }
}


bool JVRCGateEvent::isGoal() const
{
    const JVRCTask* t = task();
    if(t){
        return index() == (t->numGates() - 1);
    }
    return false;
}


JVRCTask::JVRCTask(Mapping* info)
    : info_(info)
{
    name_ = info->read<string>("name");
    Listing& eventNodes = *info->findListing("events");
    if(eventNodes.isValid()){
        for(int j=0; j < eventNodes.size(); ++j){
            Mapping* eventInfo = eventNodes[j].toMapping();
            string type = eventInfo->read<string>("type");
            if(type == "gate"){
                addEvent(new JVRCGateEvent(this, eventInfo));
            } else {
                addEvent(new JVRCEvent(type, this, eventInfo));
            }
        }
    }
}


void JVRCTask::addEvent(JVRCEvent* event)
{
    events.push_back(event);

    if(JVRCGateEvent* gate = dynamic_cast<JVRCGateEvent*>(event)){
        gate->setIndex(gates.size());
        gates.push_back(gate);
    }
}


bool JVRCTaskInfo::load(const std::string& filename)
{
    YAMLReader reader;
    MappingPtr info = reader.loadDocument(filename)->toMapping();
    const Listing& taskNodes = *info->findListing("tasks");
    if(taskNodes.isValid()){
        readTasks(taskNodes);
    }
    return true;
}


void JVRCTaskInfo::readTasks(const Listing& taskNodes)
{
    tasks.clear();
    for(int i=0; i < taskNodes.size(); ++i){
        tasks.push_back(new JVRCTask(taskNodes[i].toMapping()));
    }
}


JVRCTask* JVRCTaskInfo::findTask(const std::string& name)
{
    for(size_t i=0; i < tasks.size(); ++i){
        if(tasks[i]->name() == name){
            return tasks[i];
        }
    }
    return 0;
}
