#include "port_view.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace {
constexpr ImU32 ink = IM_COL32(222, 237, 247, 255);
const std::array<ImU32, 3> accents = {IM_COL32(245,193,95,255), IM_COL32(104,218,206,255), IM_COL32(179,157,245,255)};
const char* names[] = {"BULK / grab cranes", "LIQUID / loading arms", "CONTAINERS / gantry cranes"};
constexpr float kMinMotionSeconds = 1.2f;
constexpr float kMaxMotionSeconds = 2.8f;
constexpr float kMotionGap = 0.12f;
ImVec2 Plus(ImVec2 a, ImVec2 b) { return {a.x+b.x,a.y+b.y}; }
float Distance(ImVec2 a, ImVec2 b) { return std::hypot(a.x-b.x,a.y-b.y); }
void Ship(ImDrawList* d, ImVec2 p, int type, bool leaving) {
  const ImU32 gray = IM_COL32(164,179,190,255);
  ImVec2 hull[] = {{p.x-32,p.y-13},{p.x+21,p.y-13},{p.x+36,p.y},{p.x+21,p.y+13},{p.x-32,p.y+13}};
  d->AddConvexPolyFilled(hull,5,gray);
  d->AddRectFilled({p.x-25,p.y-9},{p.x-14,p.y+9},IM_COL32(228,237,242,255),2);
  if(type==0) {
    for(int i=0;i<3;++i) d->AddCircleFilled({p.x-5+i*10.0f,p.y},5,accents[type]);
  } else if(type==1) {
    d->AddRectFilled({p.x-8,p.y-7},{p.x+20,p.y+7},accents[type],7);
  } else {
    for(int i=0;i<3;++i) for(int j=0;j<2;++j)
      d->AddRectFilled({p.x-9+i*10.0f,p.y-9+j*10.0f},{p.x-1+i*10.0f,p.y-1+j*10.0f},accents[type],1);
  }
  if(leaving) d->AddLine({p.x-45,p.y},{p.x-36,p.y},IM_COL32(163,217,242,180),2);
}
void Crane(ImDrawList* d, ImVec2 p, int type, bool busy, float time) {
  const ImU32 color=accents[type];
  const float swing=busy ? std::sin(time*2.0f+p.x*.03f)*4.0f : 0.0f;
  d->AddRectFilled({p.x-12,p.y-6},{p.x+12,p.y+8},IM_COL32(53,68,80,255),3);
  if(type==0) {
    d->AddLine(p,{p.x+18,p.y-38},color,5);
    d->AddLine({p.x+18,p.y-38},{p.x+35+swing,p.y-18},color,4);
    d->AddTriangleFilled({p.x+29+swing,p.y-17},{p.x+41+swing,p.y-17},{p.x+35+swing,p.y-10},color);
  } else if(type==1) {
    d->AddCircleFilled(p,11,color);
    d->AddLine(p,{p.x,p.y-28},color,6);
    d->AddLine({p.x,p.y-28},{p.x+26,p.y-28},color,6);
    d->AddLine({p.x+26,p.y-28},{p.x+26,p.y-15},color,4);
  } else {
    d->AddLine({p.x-25,p.y},{p.x-25,p.y-42},color,4);
    d->AddLine({p.x+25,p.y},{p.x+25,p.y-42},color,4);
    d->AddLine({p.x-31,p.y-42},{p.x+31,p.y-42},color,6);
    d->AddLine({p.x+swing,p.y-42},{p.x+swing,p.y-19},color,2);
    d->AddRectFilled({p.x-8+swing,p.y-20},{p.x+8+swing,p.y-14},color,2);
  }
  const int lampAlpha=busy ? static_cast<int>(210+45*std::sin(time*4.0f)) : 255;
  d->AddCircleFilled({p.x-18,p.y+4},4,busy ? IM_COL32(103,230,157,lampAlpha):IM_COL32(120,132,145,255));
}
}
void PortView::Reset() { vessels_.clear(); snapshot_.clear(); clock_=0; laneHeight_=170; }
ImVec2 PortView::Target(const sea_port::ShipView& ship) const {
  const float y=65+static_cast<int>(ship.cargo_type)*laneHeight_;
  if(ship.berth>=0) return {620+ship.berth*110.0f,y+56};
  int index=0;
  for(const auto& item:snapshot_) {
    if(item.id==ship.id) break;
    if(item.cargo_type==ship.cargo_type && item.state==sea_port::ShipState::kWaiting) ++index;
  }
  return {65+(index%4)*92.0f,y+42+(index/4)*58.0f};
}
void PortView::Update(const sea_port::PortSimulation& simulation, const sea_port::DailyReport& report) {
  snapshot_=simulation.VisualSnapshot();
  laneHeight_=170;
  float nextSlot=0;
  auto schedule=[&](Vessel& v, const sea_port::ShipView& data,
                    std::vector<ImVec2> route, bool leaving, bool appearing) {
    v.data=data; v.position=route.front(); v.route=std::move(route);
    v.leaving=leaving; v.appearing=appearing; v.age=0; v.delay=nextSlot;
    float length=0;
    for(std::size_t i=1;i<v.route.size();++i)
      length+=Distance(v.route[i-1],v.route[i]);
    v.duration=v.route.size()>1 ? std::clamp(length/500.0f,kMinMotionSeconds,kMaxMotionSeconds):0;
    if(v.duration>0) nextSlot+=v.duration+kMotionGap;
  };
  auto approach=[&](const sea_port::ShipView& s){
    return 65+static_cast<int>(s.cargo_type)*laneHeight_+12;
  };
  auto aisle=[&](const sea_port::ShipView& s){
    return 65+static_cast<int>(s.cargo_type)*laneHeight_+71;
  };

  // A tick first clears the old parking and berths, then brings ships in.
  std::vector<sea_port::ShipView> sameDayCompletions;
  for(const auto& s:report.departures) {
    if(s.state==sea_port::ShipState::kCompleted && !vessels_.contains(s.id)) {
      sameDayCompletions.push_back(s);
      continue;
    }
    auto [it,inserted]=vessels_.try_emplace(s.id);
    Vessel& v=it->second;
    const float lane=approach(s);
    const ImVec2 start=inserted ? ImVec2(455,lane) : v.position;
    std::vector<ImVec2> route;
    if(!inserted && v.data.state==sea_port::ShipState::kWaiting) {
      route={start,{start.x,aisle(s)},{455,aisle(s)},{455,lane},{510,lane},{510,85+3*laneHeight_}};
    } else if(!inserted && v.data.state==sea_port::ShipState::kUnloading) {
      route={start,{start.x,lane},{510,lane},{510,85+3*laneHeight_}};
    } else {
      route={start,{510,lane},{510,85+3*laneHeight_}};
    }
    schedule(v,s,std::move(route),true,inserted);
  }

  auto updateActive=[&](const sea_port::ShipView& s) {
    auto [it,inserted]=vessels_.try_emplace(s.id);
    Vessel& v=it->second;
    const ImVec2 target=Target(s);
    const float lane=approach(s);
    const ImVec2 start=inserted ? ImVec2(455,lane) : v.position;
    if(!inserted && Distance(start,target)<1) {
      v.data=s; v.route={target}; v.position=target; v.duration=0; v.appearing=false; return;
    }
    std::vector<ImVec2> route;
    if(!inserted && v.data.state==sea_port::ShipState::kWaiting)
      route={start,{start.x,aisle(s)},{455,aisle(s)},{455,lane},{target.x,lane},target};
    else
      route={start,{455,lane},{target.x,lane},target};
    schedule(v,s,std::move(route),false,inserted);
  };
  for(const auto& s:snapshot_)
    if(s.state==sea_port::ShipState::kUnloading) updateActive(s);

  // Existing queue members fill vacated slots before new arrivals enter.
  auto updateWaiting=[&](const sea_port::ShipView& s, bool newlyArrived) {
    auto [it,inserted]=vessels_.try_emplace(s.id);
    Vessel& v=it->second;
    const ImVec2 target=Target(s);
    const ImVec2 start=inserted ? ImVec2(455,aisle(s)) : v.position;
    if(!inserted && Distance(start,target)<1) {
      v.data=s; v.route={target}; v.position=target; v.duration=0; v.appearing=false; return;
    }
    std::vector<ImVec2> route;
    if(newlyArrived) route={start,{target.x,aisle(s)},target};
    else route={start,{start.x,aisle(s)},{target.x,aisle(s)},target};
    schedule(v,s,std::move(route),false,inserted);
  };
  for(const auto& s:snapshot_)
    if(s.state==sea_port::ShipState::kWaiting && vessels_.contains(s.id)) updateWaiting(s,false);
  for(const auto& s:snapshot_)
    if(s.state==sea_port::ShipState::kWaiting && !vessels_.contains(s.id)) updateWaiting(s,true);

  // A ship that arrived and finished within one model day was absent from the
  // previous frame. Show its approach and departure as one continuous route.
  for(const auto& s:sameDayCompletions) {
    auto& v=vessels_[s.id];
    const float lane=approach(s);
    const ImVec2 berth=Target(s);
    schedule(v,s,{{455,lane},{berth.x,lane},berth,{berth.x,lane},
                  {510,lane},{510,85+3*laneHeight_}},true,true);
  }

  for(auto it=vessels_.begin();it!=vessels_.end();) {
    const bool active=std::any_of(snapshot_.begin(),snapshot_.end(),[&](const auto& s){return s.id==it->first;});
    if(!active && !it->second.leaving) it=vessels_.erase(it); else ++it;
  }
}
void PortView::Sync(const sea_port::PortSimulation& simulation) {
  Reset();
  snapshot_=simulation.VisualSnapshot();
  for(const auto& s:snapshot_) {
    auto& v=vessels_[s.id];
    v.data=s; v.position=Target(s); v.route={v.position};
  }
}
bool PortView::IsAnimating() const {
  return std::any_of(vessels_.begin(),vessels_.end(),[](const auto& item){
    const Vessel& v=item.second;
    return v.duration>0 && v.age<v.delay+v.duration;
  });
}
void PortView::Draw(const sea_port::PortSimulation& simulation,float delta,float maxHeight) {
  clock_+=delta;
  const auto& config=simulation.config();
  int slots=1;
  for(int i=0;i<3;++i) slots=std::max({slots,config.unloading_places[i],config.crane_counts[i]});
  width_=std::max(1120.0f,610+slots*110.0f);
  const float height=110+3*laneHeight_;
  ImGui::TextUnformatted("PORT MAP  |  grey ships / cargo colours / green lamp = assigned crane");
  ImGui::TextDisabled("Daily model: movements illustrate state changes. Hover a ship for cargo and status.");
  const float scale = std::clamp(std::min((ImGui::GetContentRegionAvail().x-32)/width_,
                                        (maxHeight-78)/height),0.05f,1.0f);
  ImGui::BeginChild("PortMap",ImVec2(0,height*scale+16),true,ImGuiWindowFlags_NoScrollbar|ImGuiWindowFlags_NoScrollWithMouse);
  ImVec2 origin=ImGui::GetCursorScreenPos();
  ImGui::InvisibleButton("canvas",{width_*scale,height*scale});
  auto* d=ImGui::GetWindowDrawList();
  const int firstVertex=d->VtxBuffer.Size;
  const ImVec2 clipMin=d->GetClipRectMin(), clipMax=d->GetClipRectMax();
  d->PushClipRect(origin,Plus(origin,{width_,height}),false);
  const int firstCommand=d->CmdBuffer.Size-1;
  auto point=[&](float x,float y){return Plus(origin,{x,y});};
  d->AddRectFilled(origin,point(width_,height),IM_COL32(22,77,112,255),10);
  for(float y=24;y<height;y+=33) for(float x=15;x<width_;x+=65) {
    float shift=std::sin(clock_*.8f+y*.12f+x*.03f)*6;
    d->AddLine(point(x+shift,y),point(x+22+shift,y),IM_COL32(108,188,211,80),1.5f);
    d->AddLine(point(x+25+shift,y+4),point(x+40+shift,y+4),IM_COL32(113,198,222,42),1);
  }
  d->AddRectFilled(point(10,10),point(405,height-12),IM_COL32(15,48,74,160),8);
  d->AddText(point(25,23),ink,"ANCHORAGE / WAITING PARKING");
  d->AddText(point(425,23),ink,"CHANNEL");
  d->AddText(point(610,23),ink,"TERMINALS / BERTHS");
  for(float y=62;y<height-30;y+=25) d->AddLine(point(482,y),point(482,y+12),IM_COL32(170,218,240,100),2);
  for(int type=0;type<3;++type) {
    float y=65+type*laneHeight_;
    int waiting=0;
    for(const auto& s:snapshot_) if(static_cast<int>(s.cargo_type)==type && s.state==sea_port::ShipState::kWaiting) ++waiting;
    std::string label=std::string(names[type])+"  |  waiting "+std::to_string(waiting)
        +(waiting>8 ? " (8 shown)" : "");
    d->AddText(point(25,y-9),accents[type],label.c_str());
    d->AddRectFilled(point(565,y+95),point(width_-15,y+133),IM_COL32(111,125,132,255),4);
    d->AddLine(point(565,y+95),point(width_-15,y+95),IM_COL32(223,206,140,255),3);
    for(int i=0;i<config.unloading_places[type];++i) {
      float x=620+i*110.0f;
      d->AddRect(point(x-40,y+35),point(x+43,y+78),IM_COL32(134,189,208,160),5,0,1.5f);
      d->AddRectFilled(point(x+44,y+28),point(x+54,y+108),IM_COL32(133,144,148,255),2);
      std::string berth="B"+std::to_string(i+1);
      d->AddText(point(x-12,y+138),ink,berth.c_str());
    }
    for(int i=0;i<config.crane_counts[type];++i) {
      bool busy=std::any_of(snapshot_.begin(),snapshot_.end(),[&](const auto& s){return static_cast<int>(s.cargo_type)==type && s.berth==i;});
      Crane(d,point(620+i*110.0f,y+108),type,busy,clock_);
    }
    if(config.unloading_places[type]==0 || config.crane_counts[type]==0)
      d->AddText(point(610,y+55),ink,"No service: add berths and cranes");
  }
  d->AddText(point(410,height-24),ink,"IN       OUT");
  for(auto it=vessels_.begin();it!=vessels_.end();) {
    auto& v=it->second;
    v.age+=delta;
    if(v.leaving && v.age>=v.delay+v.duration) {it=vessels_.erase(it);continue;}
    if(v.appearing && v.age<v.delay) {++it;continue;}
    if(!v.leaving && v.data.state==sea_port::ShipState::kWaiting) {
      int index=0;
      for(const auto& s:snapshot_) {
        if(s.id==v.data.id) break;
        if(s.cargo_type==v.data.cargo_type && s.state==sea_port::ShipState::kWaiting) ++index;
      }
      if(index>=8) {++it;continue;}
    }
    if(v.route.size()>1) {
      float total=0;
      for(std::size_t i=1;i<v.route.size();++i) total+=Distance(v.route[i-1],v.route[i]);
      const float progress=std::clamp((v.age-v.delay)/v.duration,0.0f,1.0f);
      const float eased=progress*progress*(3.0f-2.0f*progress);
      float travelled=eased*total;
      v.position=v.route.back();
      for(std::size_t i=1;i<v.route.size();++i) {
        float length=Distance(v.route[i-1],v.route[i]);
        if(travelled<=length && length>0) {
          float t=travelled/length;
          v.position={v.route[i-1].x+(v.route[i].x-v.route[i-1].x)*t,v.route[i-1].y+(v.route[i].y-v.route[i-1].y)*t};break;
        }
        travelled-=length;
      }
    }
    ImVec2 p=Plus(origin,{v.position.x,v.position.y+std::sin(clock_*2.1f+static_cast<float>(it->first%17))*1.4f});
    if(v.leaving) {
      const float wake=12.0f+4.0f*std::sin(clock_*5.0f);
      d->AddLine({p.x-36,p.y-5},{p.x-36-wake,p.y-10},IM_COL32(180,229,246,125),2);
      d->AddLine({p.x-36,p.y+5},{p.x-36-wake,p.y+10},IM_COL32(180,229,246,125),2);
    }
    Ship(d,p,static_cast<int>(v.data.cargo_type),v.leaving);
    std::string title=v.data.name.substr(0,13);
    d->AddText({p.x-32,p.y+18},ink,title.c_str());
    if(v.data.state==sea_port::ShipState::kUnloading) {
      float fraction=static_cast<float>(1-v.data.remaining_tons/std::max(1.0,v.data.total_tons));
      d->AddRectFilled({p.x-30,p.y-22},{p.x+30,p.y-18},IM_COL32(30,48,60,255),2);
      d->AddRectFilled({p.x-30,p.y-22},{p.x-30+60*fraction,p.y-18},IM_COL32(105,225,167,255),2);
    }
    const ImVec2 hoverMin=Plus(origin,{(v.position.x-35)*scale,(v.position.y-23)*scale});
    const ImVec2 hoverMax=Plus(origin,{(v.position.x+38)*scale,(v.position.y+33)*scale});
    if(ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(hoverMin,hoverMax)) {
      ImGui::BeginTooltip();
      ImGui::Text("%s (#%llu)",v.data.name.c_str(),static_cast<unsigned long long>(v.data.id));
      ImGui::Text("%s / %s",sea_port::ToString(v.data.cargo_type).c_str(),sea_port::ToString(v.data.state).c_str());
      ImGui::Text("Cargo remaining: %.0f / %.0f t",v.data.remaining_tons,v.data.total_tons);
      if(v.data.berth>=0) ImGui::Text("Berth / crane: %d",v.data.berth+1);
      ImGui::EndTooltip();
    }
    ++it;
  }
  for(int i=firstVertex;i<d->VtxBuffer.Size;++i) {
    auto& p=d->VtxBuffer[i].pos;
    p={origin.x+(p.x-origin.x)*scale,origin.y+(p.y-origin.y)*scale};
  }
  for(int i=firstCommand;i<d->CmdBuffer.Size;++i)
    d->CmdBuffer[i].ClipRect=ImVec4(std::max(origin.x,clipMin.x),std::max(origin.y,clipMin.y),
        std::min(origin.x+width_*scale,clipMax.x),std::min(origin.y+height*scale,clipMax.y));
  d->PopClipRect();
  ImGui::EndChild();
}
void PortView::DrawQueues(const sea_port::PortSimulation& simulation) {
  const auto& config=simulation.config();
  ImGui::Text("WAITING QUEUE  %zu / %zu places",simulation.waiting_ship_count(),config.waiting_capacity);
  if(ImGui::BeginTable("VisualQueues",3,ImGuiTableFlags_BordersInnerV|ImGuiTableFlags_SizingStretchSame)) {
    for(int type=0;type<3;++type) {
      ImGui::TableNextColumn();
      ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(accents[type]),"%s",names[type]);
      ImGui::BeginChild(type+100,ImVec2(0,120));
      bool empty=true;
      for(const auto& s:snapshot_) if(static_cast<int>(s.cargo_type)==type && s.state==sea_port::ShipState::kWaiting) {
        ImGui::BulletText("%s | %.0f t",s.name.c_str(),s.total_tons); empty=false;
      }
      if(empty) ImGui::TextDisabled("Queue empty");
      ImGui::EndChild();
    }
    ImGui::EndTable();
  }
  ImGui::TextDisabled("Queue dispatch uses backend priority (price, waiting time, urgency), not FIFO.");
}
