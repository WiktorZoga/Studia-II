# BEFORE USE: Substitute GAMENAME with the name of the game: content of <artifactId> from pom.xml file; the built jar file in target.

# run from the referee main directory; agent executables should be in agents/ dir
# Arguments (if not provided, default values are used):
#   $1 $2 - agent executables
#   $3    - number of games to run
#   $4    - number of threads to run
java -cp target/GAMENAME-1.0-SNAPSHOT.jar:target/dependency/* benchmarker.Benchmarker $1 $2 $3 $4
