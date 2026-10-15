package benchmarker;

public abstract class Confidence {

  public static double getConfidence95Interval(double avgPoints, double sumSqrPoints, int plays) {
    double var = (sumSqrPoints - avgPoints * avgPoints * plays) / (plays-1);
    return 1.9602 * Math.sqrt(var / (plays - 1));
  }

  public static double getConfidence99Interval(double avgPoints, double sumSqrPoints, int plays) {
    double var = (sumSqrPoints - avgPoints * avgPoints * plays) / (plays-1);
    return 2.5763 * Math.sqrt(var / (plays - 1));
  }

}
