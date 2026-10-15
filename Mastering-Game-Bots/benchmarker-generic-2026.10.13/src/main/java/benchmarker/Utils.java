package benchmarker;

import java.util.List;

public abstract class Utils {

  public static String encodeToOneLine(String s) {
    return (s == null ? "" : s.replace('\n', (char)31));
  }
  public static String decodeFromOneLine(String s) {
    return s.replace((char)31, '\n');
  }

  public static String concatenateListOfStrings(List<String> list) {
    StringBuilder s = new StringBuilder();
    for (String el: list) if (el != null) s.append(el);
    return s.toString();
  }

  public static String repeat(String str, int count) {return str.repeat(Math.max(0, count));}
  public static String leftPad(String str, int pad) {return repeat(" ", pad-str.length()) + str;}
  public static String rightPad(String str, int pad) {return str + repeat(" ", pad-str.length());}
}
