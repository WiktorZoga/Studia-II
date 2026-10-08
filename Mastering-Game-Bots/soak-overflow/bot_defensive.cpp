// Soak Overflow, full game. Standalone C++17, deterministic bounded work.
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
using namespace std;
struct Pos { int x=0,y=0; bool operator==(const Pos& p) const{return x==p.x&&y==p.y;} bool operator<(const Pos& p)const{return tie(x,y)<tie(p.x,p.y);} };
int man(Pos a,Pos b){return abs(a.x-b.x)+abs(a.y-b.y);}
int cheb(Pos a,Pos b){return max(abs(a.x-b.x),abs(a.y-b.y));}
struct Stats{int owner=0,cooldown=0,range=0,power=0,bombs=0;};
struct Agent{int id=0,cooldown=0,bombs=0,wet=0;Pos p;};
struct Game{
 int me=0,w=0,h=0; map<int,Stats> stats; vector<int> tiles; vector<Agent> live;
 bool inside(Pos p)const{return p.x>=0&&p.x<w&&p.y>=0&&p.y<h;}
 int tile(Pos p)const{return inside(p)?tiles[p.y*w+p.x]:-1;}
 bool floor(Pos p)const{return tile(p)==0;}
 bool own(const Agent& a)const{auto s=stats.find(a.id);return s!=stats.end()&&s->second.owner==me;}
 bool init(){
  int n;if(!(cin>>me>>n)||n<0||n>1000)return false;
  for(int i=0;i<n;++i){int id;Stats s;if(!(cin>>id>>s.owner>>s.cooldown>>s.range>>s.power>>s.bombs))return false;stats[id]=s;}
  if(!(cin>>w>>h)||w<=0||h<=0||w>1000||h>1000)return false;
  tiles.assign(w*h,-1);
  for(int i=0;i<w*h;++i){int x,y,t;if(!(cin>>x>>y>>t)||!inside({x,y}))return false;tiles[y*w+x]=t;}
  return true;
 }
 bool turn(){
  int n;if(!(cin>>n)||n<0||n>1000)return false;
  vector<Agent> next;
  for(int i=0;i<n;++i){Agent a;if(!(cin>>a.id>>a.p.x>>a.p.y>>a.cooldown>>a.bombs>>a.wet))return false;if(stats.count(a.id)&&inside(a.p))next.push_back(a);}
  int ownCount;if(!(cin>>ownCount))return false;
  live=std::move(next);sort(live.begin(),live.end(),[](const Agent&a,const Agent&b){return a.id<b.id;});return true;
 }
 vector<Pos> steps(Pos p)const{
  vector<Pos> r{p};for(Pos d:vector<Pos>{{0,-1},{-1,0},{1,0},{0,1}}){Pos q{p.x+d.x,p.y+d.y};if(floor(q))r.push_back(q);}return r;
 }
 double cover(Pos target,Pos shooter)const{
  double m=1;
  for(int axis=0;axis<2;++axis){int delta=axis?target.y-shooter.y:target.x-shooter.x;if(abs(delta)<=1)continue;
   Pos c=target;if(axis)c.y-=delta>0?1:-1;else c.x-=delta>0?1:-1;
   if(cheb(c,shooter)>1){int t=tile(c);m=min(m,t==2?.25:t==1?.5:1.);}
  }return m;
 }
 double damage(const Agent& a,Pos from,Pos target)const{
  const Stats& s=stats.at(a.id);int d=man(from,target);double r=d<=s.range?1.:d<=2*s.range?.5:0.;return std::floor(s.power*r*cover(target,from)+.5);
 }
 vector<int> distances(Pos source)const{
  vector<int>d(w*h,100000);queue<Pos>q;d[source.y*w+source.x]=0;q.push(source);
  while(!q.empty()){Pos p=q.front();q.pop();for(Pos n:steps(p)){int&i=d[n.y*w+n.x];if(i>d[p.y*w+p.x]+1){i=d[p.y*w+p.x]+1;q.push(n);}}}return d;
 }
};
struct Combat{string text="HUNKER_DOWN";double value=0;Pos blast{-100,-100};};
Combat attack(const Game& g,const Agent& a,Pos p,const map<int,Pos>& planned,const vector<Pos>& priorBombs){
 Combat best;
 if(a.cooldown==0){
  for(const Agent& e:g.live)if(!g.own(e)){
   double d=g.damage(a,p,e.p);if(d<=0)continue;
   double score=min(d,double(100-e.wet))+((e.wet+d>=100)?55:0)+.08*e.wet;
   if(score>best.value){best={"SHOOT "+to_string(e.id),score,{-100,-100}};}
  }
 }
 if(a.bombs>0){
  for(int y=max(0,p.y-4);y<=min(g.h-1,p.y+4);++y)for(int x=max(0,p.x-4);x<=min(g.w-1,p.x+4);++x){
   Pos b{x,y};if(man(p,b)>4)continue;bool safe=true;double value=0;int hits=0;
   for(const Agent& o:g.live){
    if(g.own(o)){Pos dest=o.id==a.id?p:planned.count(o.id)?planned.at(o.id):o.p;
     // Check original and planned cells: collision or a cancelled move must remain safe.
     if(cheb(o.p,b)<=1||cheb(dest,b)<=1){safe=false;break;}
    }else if(cheb(o.p,b)<=1){++hits;value+=min(30,100-o.wet)+(o.wet>=70?55:0);}
   }
   for(Pos old:priorBombs)if(cheb(old,b)<=1)value-=15;
   // Preserve scarce bombs for multiple hits or a finishing hit.
   if(safe&&hits>0&&(hits>=2||value>=70)&&value-8>best.value)best={"THROW "+to_string(x)+" "+to_string(y),value-8,b};
  }
 }return best;
}
#ifndef DEFENSIVE
#define DEFENSIVE 1
#endif
int main(){
 ios::sync_with_stdio(false);cin.tie(nullptr);Game g;if(!g.init())return 0;
 while(g.turn()){
  map<int,Pos> planned;vector<Pos> bombs;set<Pos> occupied;
  for(const Agent& a:g.live)occupied.insert(a.p);
  vector<vector<int>> enemyDistances;
  for(const Agent& e:g.live)if(!g.own(e))enemyDistances.push_back(g.distances(e.p));
  for(const Agent& a:g.live)if(g.own(a)){
   Pos bestPos=a.p;Combat bestCombat;double bestScore=-numeric_limits<double>::infinity();
   for(Pos p:g.steps(a.p)){
    if(!(p==a.p)&&occupied.count(p))continue;
    bool bombSafe=true;for(Pos b:bombs)if(cheb(p,b)<=1)bombSafe=false;if(!bombSafe)continue;
    Combat c=attack(g,a,p,planned,bombs);double risk=0;int nearest=100000;
    for(const auto& ds:enemyDistances)nearest=min(nearest,ds[p.y*g.w+p.x]);
    for(const Agent& e:g.live)if(!g.own(e)&&e.cooldown<=1){
     double threat=g.damage(e,e.p,p);
     for(Pos ep:g.steps(e.p))threat=max(threat,g.damage(e,ep,p));
     risk+=threat;
    }
    // Position toward reachable opponents and the centre; leave cover only when worthwhile.
    double approach=-.9*min(nearest,1000);double centre=-.12*man(p,{g.w/2,g.h/2});
    double exposure=(DEFENSIVE?1.15:.58)+(a.wet>=70?.6:0);
    if(c.text=="HUNKER_DOWN")risk*=.75;
    double score=c.value-risk*exposure+approach+centre-(p==a.p?0:.12);
    if(score>bestScore+1e-9){bestScore=score;bestPos=p;bestCombat=c;}
   }
   planned[a.id]=bestPos;occupied.insert(bestPos);
   if(bestCombat.blast.x>=0)bombs.push_back(bestCombat.blast);
   cout<<a.id<<';';if(!(bestPos==a.p))cout<<"MOVE "<<bestPos.x<<' '<<bestPos.y<<';';
   cout<<bestCombat.text<<'\n';
  }
  cout.flush();
 }return 0;
}
