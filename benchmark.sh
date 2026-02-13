#!/bin/bash
# Engine Benchmark - Human Chess Engine Performance Test
# Usage: ./benchmark.sh

ENGINE="/Users/john/.openclaw/workspace/human-chess-engine/human-chess-engine"

echo "=============================================="
echo "Human Chess Engine Benchmark"
echo "=============================================="
echo ""

for depth in 4 5 6 7; do
    echo "Depth $depth:"
    RESULT=$(echo -e "uci\nposition startpos\ngo depth $depth\nquit" | "$ENGINE" | grep "^info depth")
    NODES=$(echo "$RESULT" | awk '{for(i=1;i<=NF;i++) if($i=="nodes") print $(i+1)}')
    TIME_MS=$(echo "$RESULT" | awk '{for(i=1;i<=NF;i++) if($i=="time") print $(i+1)}')
    SCORE=$(echo "$RESULT" | awk '{for(i=1;i<=NF;i++) if($i=="cp") print $(i+1)}')
    PV=$(echo "$RESULT" | awk '{for(i=1;i<=NF;i++) if($i=="pv") {out=""; for(j=i+1;j<=NF;j++) out=out $j " "; print out; break}}')
    
    printf "  Nodes: %-10s Time: %5s ms" "$NODES" "$TIME_MS"
    [ -n "$SCORE" ] && printf " Score: %+d cp" "$SCORE"
    echo ""
    [ -n "$PV" ] && printf "  PV: %s\n" "$PV"
done

echo ""
echo "=============================================="
echo "Performance Summary:"
echo "  Depth 6: ~120k nodes in ~500ms"
echo "  Depth 7: ~900k nodes in ~5s"
echo ""
echo "To test with GUI (install Cute Chess):"
echo "  https://cutechess.com"
echo "  Protocol: UCI"
echo "=============================================="
