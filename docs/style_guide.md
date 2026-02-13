# Human Chess Engine - Style Guide

## Evaluation Philosophy

This engine evaluates positions using **human concepts**, not raw calculation.

### Silman's Imbalance Theory (Core Framework)

From Jeremy Silman's "How to Reassess Your Chess", we evaluate positions by identifying **imbalances** — significant differences between sides.

### The 10 Major Imbalances

1. **Material** - Piece counts and exchanges
2. **Pawn Structure** - Weak pawns, passed pawns, doubled pawns
3. **Space** - Territorial control
4. **Superior Minor Piece** - Bishop vs Knight evaluation
5. **Control of Key Files** - Open files for rooks
6. **Control of Weak Squares/Holes** - Knight outposts
7. **Lead in Development** - Early game initiative
8. **Initiative** - "Calling the shots"
9. **King Safety** - Attack opportunities
10. **Statics vs Dynamics** - Long-term vs temporary advantages

### Russian Chess School Additions

From Konstantin Sakaev's "Russian Chess School 2.0":

- **Prophylaxis** - Always ask "What does my opponent want?"
- **Systematic Evaluation** - Reduce candidates through knowledge
- **Centralization** - Kings dominate endgames
- **Small Advantages** - Patient accumulation

## Style Profiles

### Classical (Karpovian)
- High weight: Structure, prophylaxis, patience
- Low weight: Initiative, tactics
- Plays: Patient grinding, small improvements

### Attacking (Vukovic/Sokolov)
- High weight: Initiative, king safety, sacrifices
- Low weight: Material, static advantages
- Plays: Initiative-seeking, complications

### Tactical
- High weight: Initiative, piece activity
- Low weight: Pawn structure, prophylaxis
- Plays: Sharp positions, combinations

### Positional (Silman)
- Balanced: All imbalances matter
- Focus: Identify and exploit dominant imbalance
- Plays: Structural exploitation

### Technical (Shereshevsky)
- High weight: King activity, pawn conversion
- Low weight: Initiative, dynamics
- Plays: Exact conversion, fortress building

## Evaluation Formula (Simplified)

```
score = material_diff * W_material
      + piece_activity_diff * W_activity  
      + pawn_structure_diff * W_pawns
      + space_diff * W_space
      + initiative_bonus * W_initiative
      + king_safety_diff * W_safety
```

Where weights (W_*) depend on selected style.

## Key Concepts

### Statics vs Dynamics

- **Statics** - Permanent advantages (weak pawns, bad bishops, space)
- **Dynamics** - Temporary advantages (initiative, development lead)

Rule: Use dynamics quickly; accumulate statics patiently.

### When to Calculate

From Kotov's "Play Like a Grandmaster":
1. Identify candidate moves (2-5) using positional judgment
2. Calculate only those candidates
3. Don't calculate everything!

Our search mirrors this: **selective exploration**, not exhaustive search.

## References

- Silman, Jeremy. "How to Reassess Your Chess" (2007)
- Sakaev, Konstantin. "Russian Chess School 2.0 Vol 1 & 2" (2017)
- Shereshevsky, M.I. "Endgame Strategy" (1985)
- Kotov, Alexander. "Play Like a Grandmaster" (1978)
- Vukovic, Vladimir. "The Art of Attack" (1958)

---

*"Talk to the board and it will tell you what to do."* — Jeremy Silman
