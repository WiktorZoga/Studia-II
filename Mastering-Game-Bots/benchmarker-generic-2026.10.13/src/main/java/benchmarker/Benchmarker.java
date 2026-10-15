package benchmarker;

import java.io.BufferedWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardOpenOption;
import java.util.Locale;
import java.util.concurrent.ArrayBlockingQueue;

/**
 * By MSz (& JKo) 2026
 */
public class Benchmarker {
  static int NUM_THREADS = 1;
  static String[] AGENTS = {"test","test"};
  static boolean[] AGENTS_LOG = {true,true};
  static boolean SUMMARIES_LOG = true;

  static final long BASE_SEED = 1;
  static int NUM_PLAYS = 10;

  static Path AGENTS_PATH = Path.of("./agents/");
  static Path LOGS_PATH = Path.of("./log/");

  ArrayBlockingQueue<Task> taskQueue = new ArrayBlockingQueue<>(NUM_THREADS);
  WorkerProcess[] workerProcesses = new WorkerProcess[NUM_THREADS];
  Results[] results;
  int playedCount;
  long startTime;

  static class Results {
    double points, sumSqrPoints;
    int sumScores, wins, draws, loses, errors;
  }

  private void appendLog(String fileName, String log) {
    Path logFile = LOGS_PATH.resolve(fileName + ".log");
    try (BufferedWriter writer = Files.newBufferedWriter(logFile,StandardOpenOption.CREATE, StandardOpenOption.APPEND)) {
      writer.write(log);
    } catch (IOException e) {
      System.err.println("Unable to write log to " + logFile + ": " + e);
      e.printStackTrace();
    }
  }

  public synchronized void reportResult(Task task, int[] scores, String[] agentLogs, String summaryLog) {
    playedCount++;
    int numAgents = task.perm.length;
    for (int p = 0; p < numAgents; p++) {
      int a = task.perm[p];
      if (scores[p] < 0) {
        System.err.println("Error of " + AGENTS[a] + " for " + task + " (score " + scores[p] + ")");
        results[a].errors++;
      } else {
        results[a].sumScores += scores[p];
        double points = 0.0;
        for (int other = 0; other < numAgents; other++) {
          if (other == p) continue;
          if (scores[p] > scores[other]) {results[a].wins++; points += 1.0;} else
          if (scores[p] < scores[other]) results[a].loses++; else
            {results[a].draws++; points += 0.5;}
        }
        points /= (numAgents-1);
        results[a].points += points;
        results[a].sumSqrPoints += points * points;
      }
    }
    for (int p = 0; p < numAgents; p++) {
      if (!agentLogs[p].isEmpty()) appendLog(AGENTS[task.perm[p]],"*** " + task + "\n" + agentLogs[p] + "\n");
    }
    if (!summaryLog.isEmpty()) appendLog("summary", "*** " + task + "\n" + summaryLog + "\n");
    StringBuilder s = new StringBuilder();
    s.append(task).append(": ").append(scores[0]);
    for (int p = 1; p < numAgents; p++) s.append(" vs ").append(scores[p]);
    System.out.println(s);
    if (playedCount % 100 == 0 && playedCount < NUM_PLAYS) printCurrentResults(false);
  }

  private void printCurrentResults(boolean finalReport) {
    StringBuilder s = new StringBuilder();
    if (finalReport) {
      s.append("\n*** ").append(AGENTS[0]);
      for (int a = 1; a < AGENTS.length; a++) s.append(" vs ").append(AGENTS[a]);
      s.append(" completed in ").append((System.currentTimeMillis() - startTime) / 1000.0).append("s\n");
    } else {
      s.append("\n*** Temporary results after ").append(playedCount).append(" plays\n");
    }
    for (int a = 0; a < AGENTS.length; a++) {
      double winRatio = results[a].points / playedCount;
      s.append(AGENTS[a]).append(": ").append(Math.round(winRatio * 100.0)).append("%");
      double confidenceInterval = Confidence.getConfidence95Interval(winRatio, results[a].sumSqrPoints, playedCount);
      s.append("  ±").append(Utils.rightPad(String.format(Locale.US, "%.2f", Math.round(1000.0 * confidenceInterval) / 10.0), 5));
      s.append("   ").append(results[a].wins).append(" : ").append(results[a].draws).append(" : ").append(results[a].loses);
      s.append("  (").append(results[a].errors).append(" errors)");
      s.append("  ").append(playedCount).append(" plays   ");
      s.append("  avgScore ").append(String.format(Locale.US, "%.2f", (double)results[a].sumScores / playedCount)).append("  ");
      s.append(winRatio - confidenceInterval > 0.5 ? "WON" : ((winRatio + confidenceInterval < 0.5) ? "LOST" : "DRAW")).append("\n");
    }
    System.out.println(s);
  }


  public static class Task {
    final int[] perm;
    final long seed;
    Task() {seed=0;perm=new int[0];}
    Task(int[] perm, long seed) {
      this.perm = perm;
      this.seed = seed;
    }
    @Override public String toString() {
      if (perm == null) return "";
      StringBuilder s = new StringBuilder();
      s.append(seed).append("|").append(AGENTS[perm[0]]);
      for (int p = 1; p < perm.length; p++) s.append(" vs ").append(AGENTS[perm[p]]);
      return s.toString();
    }
  }

  void run() throws InterruptedException {
    startTime = System.currentTimeMillis();
    playedCount = 0;
    results = new Results[AGENTS.length];
    for (int a = 0; a < AGENTS.length; a++) results[a] = new Results();

    for (int i = 0; i < NUM_THREADS; i++) {
      workerProcesses[i] = new WorkerProcess(this);
      workerProcesses[i].start();
    }
    System.out.println("Running...");

    final int[][][] PERMUTATIONS = {// Symmetric
      {{0}},// One player
      {{0,1},{1,0}},// Two players
      {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}},// Three players
    };
//    final int[][][] PERMUTATIONS = {// Nonsymmetric
//      {{0}},
//      {{0,1}},
//      {{0,1,2}},
//    };

    long seed = BASE_SEED;
    for (int playCount = 0; playCount < NUM_PLAYS; playCount += PERMUTATIONS[AGENTS.length-1].length, seed++) {
      for (int[] perm: PERMUTATIONS[AGENTS.length-1]) {
        taskQueue.put(new Task(perm, seed));
      }
    }

    for (int i = 0; i < NUM_THREADS; i++) taskQueue.put(new Task());
    for (int i = 0; i < NUM_THREADS; i++) workerProcesses[i].join();

    printCurrentResults(true);
  }

  public static void main(String[] args) throws InterruptedException {
    for (int a = 0; a < AGENTS.length; a++)
      if (args.length > a) AGENTS[a] = args[a];
    if (args.length > AGENTS.length) NUM_PLAYS = Integer.parseInt(args[AGENTS.length]);
    if (args.length > AGENTS.length+1) NUM_THREADS = Integer.parseInt(args[AGENTS.length+1]);
    new Benchmarker().run();
  }
}
