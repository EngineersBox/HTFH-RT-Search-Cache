import glob, re, os
from datetime import datetime

def getDateTime(path: str) -> str:
    return datetime.strptime(re.search("(\d+-){2}\d+_(\d+-){2}\d+", path).group(0), "%Y-%m-%d_%H-%M-%S")

def main(logsPath: str, maxLogs: int) -> None:
    logs = glob.glob(logsPath + "/*.log")
    if (len(logs) <= maxLogs):
        print("Found {} logs, below threshold. Skipping".format(len(logs)))
        return
    logFileMap = {}
    for logPath in logs:
        logFileMap[getDateTime(logPath)] = logPath
    sortedDatetimes = sorted(logFileMap.keys())
    for dt in sortedDatetimes[:len(sortedDatetimes) - maxLogs]:
        os.remove(logFileMap[dt])
    print("Removed {} logs:\n - {}".format(
        len(sortedDatetimes) - maxLogs,
        "\n - ".join([logFileMap[dt] for dt in sortedDatetimes[:len(sortedDatetimes) - maxLogs]])
    ))

MAX_LOGS = 20
LOGS_PATH = "logs"

if __name__ == "__main__":
    main(LOGS_PATH, MAX_LOGS)
