package benchmarker;

import com.codingame.gameengine.runner.MultiplayerGameRunner;
import com.codingame.gameengine.runner.simulate.GameResult;

public abstract class RefereeCaller {
  public static GameResult runOnePlay(String[] agents, long seed, String refereeArgStr) throws Exception {
    MultiplayerGameRunner gameRunner = new MultiplayerGameRunner();

    // Reset static variables if needed

    gameRunner.setLeagueLevel(5);
  
    gameRunner.setSeed(seed);
    for (String agent: agents) gameRunner.addAgent(agent);
    return gameRunner.simulate();
  }
}
