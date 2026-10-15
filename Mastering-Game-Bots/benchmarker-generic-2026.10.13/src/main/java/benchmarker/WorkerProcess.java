package benchmarker;

import com.codingame.gameengine.runner.simulate.GameResult;

import java.io.*;
import java.util.List;
import java.util.logging.Handler;
import java.util.logging.Level;
import java.util.logging.LogManager;
import java.util.logging.Logger;

public class WorkerProcess extends Thread {
  static final char SPLIT_CHAR = (char)30;
  static final String SPLIT_CHAR_STR = Character.toString(SPLIT_CHAR);
  static final char LINE_SPLIT_CHAR = (char)31;

  public static String exceptionToString(Exception e) {
    StringWriter sw = new StringWriter();
    e.printStackTrace(new PrintWriter(sw));
    return sw.toString();
  }
  public static String concatenateListOfStrings(List<String> list) {
    StringBuilder s = new StringBuilder();
    for (String el: list) if (el != null) s.append(el);
    return s.toString();
  }
  public static String encodeToOneLine(String s) {
    return (s == null ? "" : s.replace('\n', LINE_SPLIT_CHAR));
  }
  public static String decodeFromOneLine(String s) {
    return s.replace(LINE_SPLIT_CHAR, '\n');
  }

  private static String runOnePlay(long seed, String[] agents, int[] optLogs, String refereeArgStr) throws Exception {
    long startTime = System.currentTimeMillis();
    GameResult result = RefereeCaller.runOnePlay(agents, seed, refereeArgStr);
    long endTime = System.currentTimeMillis();

    // P = agents.length; split by Config.SPLIT_CHAR
    // time score1 score2 ... scoreP log1 log2 ... logP logReferee
    StringBuilder s = new StringBuilder();
    s.append(endTime - startTime).append(SPLIT_CHAR);
    boolean[] crashed = new boolean[agents.length];
    for (int p = 0; p < agents.length; p++) {
      int score;
      try {
        score = result.scores.get(p);
      } catch (Exception e) {
        throw new Exception("Missing " + p + " in score: " + result.scores.get(0));
      }
      crashed[p] = (score < 0);
      s.append(score).append(SPLIT_CHAR);
    }
    boolean anyCrashed = false;
    for (int p = 0; p < agents.length; p++) {
      anyCrashed = anyCrashed || crashed[p];
      if (optLogs[p] == 2 || (optLogs[p] == 1 && crashed[p])) s.append(concatenateListOfStrings(result.errors.get(Integer.toString(p))));
      s.append(SPLIT_CHAR);
    }
    if (optLogs[agents.length] == 2 || (optLogs[agents.length] == 1 && anyCrashed)) s.append(concatenateListOfStrings(result.summaries));
    return s.toString();
  }

  // ******************************************************************************************************************

  static final String JAVA_BIN = System.getProperty("java.home") + File.separator + "bin" + File.separator + "java";
  static final String CLASSPATH = System.getProperty("java.class.path");
  public static final String[] REFEREE_CMD = new String[]{JAVA_BIN,
    "--add-opens", "java.base/java.lang=ALL-UNNAMED", "-XX:-UsePerfData",
    "-cp", CLASSPATH, WorkerProcess.class.getName(), "w"};

  public static Process exec() throws IOException {
    return Runtime.getRuntime().exec(REFEREE_CMD);
  }

  /**
   * In a separate process
   */
  public static void main(String[] params) {
    if (params.length != 1 || !params[0].equals("w")) {
      throw new IllegalArgumentException("This is worker's main, not for manual run (args " + params.length + ")");
    }
    try {
      Logger rootLogger = LogManager.getLogManager().getLogger("");
      rootLogger.setLevel(Level.OFF);
      for (Handler h : rootLogger.getHandlers()) rootLogger.removeHandler(h);
      BufferedReader input = new BufferedReader(new InputStreamReader(System.in));
      System.gc();
      String line;
      while ((line = input.readLine()) != null) {
        if (line.equals("q")) return;
        String[] args = line.split("\\|", -1);
        int index = 0;
        int numAgents = Integer.parseInt(args[index++]);
        long seed = Long.parseLong(args[index++]);
        String[] agentCmds = new String[numAgents];
        int[] optLogs = new int[numAgents+1];
        for (int p = 0; p < agentCmds.length; p++) {
          agentCmds[p] = args[index++];
          optLogs[p] = Integer.parseInt(args[index++]);
        }
        optLogs[agentCmds.length] = Integer.parseInt(args[index++]);
        String refereeOptions = args[index];
        try {
          String resultStr = runOnePlay(seed, agentCmds, optLogs, refereeOptions);
          System.out.println(encodeToOneLine(resultStr));
        } catch (Exception e) {
          System.out.println("runOnePlay exception for: " + line + encodeToOneLine("\n" + exceptionToString(e)));
        }
        System.out.flush();
      }
    } catch (Exception e) {
      System.out.println("WorkerProcess exception:" + encodeToOneLine("\n" + exceptionToString(e)));
      System.out.flush();
    }
  }

  // ******************************************************************************************************************

  final Benchmarker main;
  Process workerProcess;
  BufferedWriter processInput;
  BufferedReader processOutput;

  WorkerProcess(Benchmarker main) {
    this.main = main;
  }

  void startProcess() throws IOException {
    workerProcess = WorkerProcess.exec();
    processInput = new BufferedWriter(new OutputStreamWriter(workerProcess.getOutputStream()));
    processOutput = new BufferedReader(new InputStreamReader(workerProcess.getInputStream()));
  }

  void closeProcess() throws IOException, InterruptedException {
    if (workerProcess == null) return;
    try {
      processInput.write("q\n");
      processInput.close();
      workerProcess.waitFor();
    } finally {
      workerProcess = null;
    }
  }

  @Override public void run() {
    try {
      startProcess();
      while (true) {
        Benchmarker.Task task = main.taskQueue.take();
        int numAgents = task.perm.length;
        if (numAgents == 0) break;

        StringBuilder s = new StringBuilder();
        s.append(numAgents).append("|").append(task.seed);
        for (int p = 0; p < numAgents; p++) {
          s.append("|").append(Benchmarker.AGENTS_PATH.resolve(Benchmarker.AGENTS[task.perm[p]]));
          s.append("|").append(Benchmarker.AGENTS_LOG[task.perm[p]] ? "2" : "0");
        }
        s.append("|").append(Benchmarker.SUMMARIES_LOG ? "2" : "0").append("|\n");
        processInput.write(s.toString());
        processInput.flush();

        String outStr = processOutput.readLine();
        String[] outTokens = outStr.split(SPLIT_CHAR_STR,-1);
        if (outTokens.length != 2*numAgents + 2) throw new Exception("Invalid token counts: " + outTokens.length + " in: " + outStr);
        int[] scores = new int[numAgents];
        String[] logs = new String[numAgents];
        for (int p = 0; p < numAgents; p++) {
          scores[p] = Integer.parseInt(outTokens[1+p]);
          logs[p] = Utils.decodeFromOneLine(outTokens[1+numAgents+p]);
        }
        String summaries = Utils.decodeFromOneLine(outTokens[1+2*numAgents]);
        main.reportResult(task, scores, logs, summaries);
      }
    } catch (Exception e) {
      e.printStackTrace();
    }
    try {
      closeProcess();
    } catch (Exception ignored) {}
  }

}
