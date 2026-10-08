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
int main(){ios::sync_with_stdio(false);cin.tie(nullptr);Game g;if(!g.init())return 0;
 while(g.turn()){for(const Agent&a:g.live)if(g.own(a)){const Agent* target=nullptr;
 for(const Agent&e:g.live)if(!g.own(e)&&(!target||e.wet>target->wet))target=&e;
 cout<<a.id<<';';if(target&&a.cooldown==0)cout<<"SHOOT "<<target->id;else cout<<"MESSAGE waiting";cout<<'\n';}cout.flush();}return 0;}
