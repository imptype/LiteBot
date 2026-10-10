#!/bin/sh
# usage: monitor [process-name]

# SETTINGS
NAME=${1:-bot}
SHOW_COST=1       # 1 = show electricity table, 0 = hide it
HEADER_EVERY=25   # seconds (rows) before the header repeats (min 1)
WATTS=15          # assumed W per fully used logical CPU
PRICE=0.25        # £ per kWh

# SETUP
HZ=$(getconf CLK_TCK)
MEMTOTAL=$(awk '/MemTotal/{print $2}' /proc/meminfo)
CMD=$NAME

setfmt() {
  CW=${#CMD}; [ "$CW" -lt 3 ] && CW=3
  LEFT="%-5s %-5s %6s %5s %4s %6s %7s %8s %-${CW}s"
  RIGHT=' | %7s %9s %9s %9s %9s %9s'
  if [ "$SHOW_COST" = 1 ]; then FMT="$LEFT$RIGHT"; else FMT="$LEFT"; fi
}

# HEADER
header() {
  if [ "$SHOW_COST" = 1 ]; then
    h=$(printf "$FMT" PID PPID %CPU %CPUA %MEM RSS VSZ ELAPSED CMD WATTS ENERGY perHour perDay perMonth perYear)
  else
    h=$(printf "$FMT" PID PPID %CPU %CPUA %MEM RSS VSZ ELAPSED CMD)
  fi
  [ "$any" = 1 ] && printf '%*s\n' "${#h}" '' | tr ' ' '-'
  echo "$h"
  any=1
}

# LOOP
setfmt
PID=""; prev=""; i=0; any=0
while :; do
  sleep 1
  [ -z "$PID" ] && PID=$(pgrep -x "$NAME" | head -n1)
  st=""
  [ -n "$PID" ] && st=$(sed 's/.*) //' /proc/$PID/stat 2>/dev/null)

  # IDLE
  if [ -z "$st" ]; then
    PID=""; prev=""
    continue
  fi

  # START
  cur="$st $(cut -d' ' -f1 /proc/uptime)"
  if [ -z "$prev" ]; then
    c=$(ps -p "$PID" -o args= 2>/dev/null); [ -n "$c" ] && CMD=$c
    setfmt
    prev=$cur; i=0
    continue
  fi

  # ROW
  rss=$(awk '/VmRSS/{print $2}' /proc/$PID/status 2>/dev/null)
  [ $((i % HEADER_EVERY)) -eq 0 ] && header
  i=$((i + 1))

  echo "$prev|$cur|$rss" | awk -F'|' -v pid="$PID" -v cmd="$CMD" -v mt="$MEMTOTAL" -v hz="$HZ" -v w="$WATTS" -v p="$PRICE" -v fmt="$FMT" -v cost="$SHOW_COST" '
    function pence(x) {
      x = sprintf("%.4f", x)
      sub(/0+$/, "", x); sub(/\.$/, "", x)
      return x "p"
    }
    {
      na = split($1, a, " "); nb = split($2, b, " ")
      t0 = a[12] + a[13];  t1 = b[12] + b[13]
      now = (t1 - t0) / hz / (b[nb] - a[na]) * 100
      age = b[nb] - b[20] / hz
      cpu = t1 / hz
      avg = (age > 0) ? cpu / age * 100 : 0
      watts = avg / 100 * w
      ph = watts / 1000 * p * 100

      s = int(age); d = int(s / 86400); h = int(s % 86400 / 3600)
      m = int(s % 3600 / 60); sec = s % 60
      if (d > 0)      el = sprintf("%d-%02d:%02d:%02d", d, h, m, sec)
      else if (h > 0) el = sprintf("%d:%02d:%02d", h, m, sec)
      else            el = sprintf("%02d:%02d", m, sec)

      l1 = sprintf("%.2f", now); l2 = sprintf("%.2f", avg)
      l3 = sprintf("%.1f", $3 * 100 / mt); l4 = sprintf("%d", b[21] / 1024)
      if (cost == 1)
        printf fmt "\n", pid, b[2], l1, l2, l3, $3, l4, el, cmd,
          sprintf("%.3fW", watts), sprintf("%.4fWh", cpu * w / 3600),
          pence(ph), pence(ph * 24), pence(ph * 720), pence(ph * 8760)
      else
        printf fmt "\n", pid, b[2], l1, l2, l3, $3, l4, el, cmd
    }'
  prev=$cur
done