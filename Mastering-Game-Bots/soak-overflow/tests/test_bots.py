"""Subprocess contract tests with hand-checked board positions (no strategy mocks)."""
import pathlib, re, subprocess, tempfile, unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]


def game(stats, turns, width=9, height=5, covers=(), me=0):
    tiles = {(x,y): t for x,y,t in covers}
    fields = [str(me), str(len(stats))]
    fields += [' '.join(map(str,s)) for s in stats]
    fields += [f'{width} {height}']
    fields += [f'{x} {y} {tiles.get((x,y),0)}' for y in range(height) for x in range(width)]
    for agents in turns:
        fields += [str(len(agents))] + [' '.join(map(str,a)) for a in agents]
        own = {s[0] for s in stats if s[1] == me}
        fields += [str(sum(a[0] in own for a in agents))]
    return '\n'.join(fields)+'\n'

class Bots(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.bins = {}
        for name in ['bot', 'bot_defensive', 'training/wood1', 'training/wood2', 'training/wood3', 'training/wood4']:
            out = pathlib.Path(cls.tmp.name)/name.replace('/','_')
            subprocess.run(['c++','-std=c++17','-O2','-Wall','-Wextra','-pedantic',str(ROOT/(name+'.cpp')),'-o',str(out)],check=True)
            cls.bins[name] = out
    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()
    def run_bot(self,name,data):
        p = subprocess.run([str(self.bins[name])],input=data,text=True,capture_output=True,timeout=2)
        self.assertEqual(p.returncode,0,p.stderr)
        return p.stdout.strip().splitlines() if p.stdout.strip() else []
    def test_live_ownership_sparse_ids_and_whitespace(self):
        # Contiguous-id or fixed-team assumptions lose id 91 or command a vanished agent.
        s=[(91,1,2,3,20,0),(7,0,2,3,20,0),(42,1,2,3,20,0)]
        t=[[(7,7,2,0,0,0),(42,1,3,1,0,0),(91,1,1,1,0,0)],[(7,7,2,0,0,0),(91,1,1,1,0,0)]]
        data=game(s,t,me=1)
        for n in ['bot','bot_defensive']:
            out=self.run_bot(n,' \t '.join(data.split()))
            self.assertEqual([int(x.split(';')[0]) for x in out],[42,91,91])
    def test_truncated_turn_has_no_partial_commands(self):
        data=game([(91,0,2,3,20,0)],[])+ '1\n91 1 1 0'
        self.assertEqual(self.run_bot('bot',data),[])
        self.assertEqual(self.run_bot('bot',''),[])
    def test_cooldown_and_out_of_range(self):
        s=[(11,0,2,2,20,0),(99,1,2,2,20,0)]
        for cooldown,x in [(1,3),(0,8)]:
            out=self.run_bot('bot',game(s,[[(11,0,2,cooldown,0,0),(99,x,2,0,0,0)]]))
            self.assertEqual(len(out),1)
            self.assertNotIn('SHOOT',out[0])
    def test_shoots_available_enemy(self):
        s=[(11,0,2,3,20,0),(99,1,2,3,20,0)]
        out=self.run_bot('bot',game(s,[[(11,1,2,0,0,0),(99,4,2,1,0,90)]]))
        self.assertEqual(len(out),1)
        self.assertIn('SHOOT 99',out[0])
    def test_enemy_group_bomb_and_friend_protection(self):
        s=[(11,0,2,1,10,3),(22,0,2,1,10,0),(99,1,2,3,20,0),(77,1,2,3,20,0)]
        a=[(11,1,2,1,3,0),(22,8,4,1,0,0),(99,4,1,1,0,70),(77,4,3,1,0,70)]
        for n in ['bot','training/wood4']:
            out=self.run_bot(n,game(s,[a]))
            self.assertEqual(len(out),2)
            self.assertIn('THROW ',out[0])
            bomb=next(c for c in out[0].split(';') if c.startswith('THROW '))
            _,x,y=bomb.split()
            self.assertIn((int(x),int(y)),[(3,2),(4,2),(5,2)])
            a2=[a[0],(22,4,2,1,0,0),a[2],a[3]]
            out=self.run_bot(n,game(s,[a2]))
            for line in out:
                for action in line.split(';'):
                    if action.startswith('THROW '):
                        _,x,y=action.split()
                        self.assertGreater(max(abs(int(x)-4),abs(int(y)-2)),1)
    def test_movement_stays_on_adjacent_free_floor_and_shot_range(self):
        # A path-through-cover or projected-range regression emits an illegal command.
        stats=[(11,0,2,2,20,0),(99,1,2,2,20,0)]
        for enemy_x in [3,4,5,8]:
            data=game(stats,[[(11,1,2,0,0,0),(99,enemy_x,2,2,0,0)]],covers=[(1,1,2),(2,2,1)])
            for name in ['bot','bot_defensive']:
                lines=self.run_bot(name,data)
                self.assertEqual(len(lines),1)
                position=(1,2)
                combats=0
                for command in lines[0].split(';')[1:]:
                    if command.startswith('MOVE '):
                        _,x,y=command.split();position=(int(x),int(y))
                        self.assertIn(position,[(0,2),(1,3)])
                    else:
                        combats+=1
                        self.assertRegex(command,r'^(SHOOT 99|HUNKER_DOWN)$')
                    if command.startswith('SHOOT '):
                        self.assertLessEqual(abs(position[0]-enemy_x)+abs(position[1]-2),4)
                self.assertEqual(combats,1)
    def test_partial_initialization_exits_cleanly(self):
        for name in self.bins:
            self.assertEqual(self.run_bot(name,'0 1 8 0 2'),[])

    def test_wood1_destinations(self):
        s=[(8,0,0,6,50,0),(19,0,0,6,50,0)]
        out=self.run_bot('training/wood1',game(s,[[(8,0,0,0,0,0),(19,0,4,0,0,0)]],width=8))
        self.assertEqual(out,['8;MOVE 6 1','19;MOVE 6 3'])
    def test_wood2_wettest_not_nearest(self):
        s=[(8,0,0,6,50,0),(71,1,0,6,100,0),(43,1,0,6,100,0)]
        out=self.run_bot('training/wood2',game(s,[[(8,3,2,0,0,0),(71,2,1,0,0,80),(43,5,3,0,0,90)]]))
        self.assertEqual(out,['8;SHOOT 43'])
    def test_wood3_cover_and_target(self):
        s=[(1,0,0,6,50,0),(2,0,0,6,50,0)]+[(i,1,0,6,100,0) for i in range(3,7)]
        a=[(1,0,2,0,0,0),(2,12,2,0,0,0),(3,4,1,0,0,0),(4,4,3,0,0,0),(5,8,1,0,0,0),(6,8,3,0,0,0)]
        covers=[(1,1,2),(1,3,1),(11,1,1),(11,3,2),(3,1,2),(3,3,1),(9,1,0),(9,3,2)]
        out=self.run_bot('training/wood3',game(s,[a],width=13,covers=covers))
        self.assertEqual(out,['1;MOVE 0 1;SHOOT 4','2;MOVE 12 3;SHOOT 5'])
    def test_wood3_vertical_cover_must_not_hide_tutorial_target(self):
        # Optional cover between vertically separated enemies can tie actual damage,
        # but tutorial selects the foe behind the lower horizontal barricade.
        stats=[(1,0,0,6,50,0),(2,0,0,6,50,0)]+[(i,1,0,6,100,0) for i in range(3,7)]
        agents=[(1,0,2,0,0,0),(2,12,2,0,0,0),(3,4,1,0,0,0),(4,4,3,0,0,0),(5,8,1,0,0,0),(6,8,3,0,0,0)]
        cases=[
          ([(1,1,1),(9,1,2),(11,1,2),(4,2,1),(8,2,2),(1,3,2),(3,3,2),(11,3,1),(4,4,1),(8,4,2)],
           ['1;MOVE 0 3;SHOOT 3','2;MOVE 12 1;SHOOT 6']),
          ([(1,1,2),(3,1,2),(6,1,2),(9,1,2),(11,1,1),(4,2,2),(8,2,1),(1,3,1),(3,3,1),(6,3,2),(9,3,1),(11,3,2)],
           ['1;MOVE 0 1;SHOOT 4','2;MOVE 12 3;SHOOT 6'])]
        for covers,want in cases:
            with self.subTest(covers=covers):
                self.assertEqual(self.run_bot('training/wood3',game(stats,[agents],width=13,covers=covers)),want)

if __name__=='__main__': unittest.main()
