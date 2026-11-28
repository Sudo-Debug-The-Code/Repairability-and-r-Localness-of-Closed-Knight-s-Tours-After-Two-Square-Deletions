import json
from collections import Counter, deque
from itertools import combinations
from pathlib import Path


def knight_move(a, b, side):
    r1, c1 = divmod(a, side)
    r2, c2 = divmod(b, side)
    return sorted((abs(r1 - r2), abs(c1 - c2))) == [1, 2]


def tour_edges(tour, squares, legal):
    if len(tour) != len(squares) or set(tour) != squares:
        raise ValueError('Tour does not visit every required square exactly once')
    steps = zip(tour, tour[1:] + tour[:1])
    edges = {tuple(sorted(step)) for step in steps}
    if not edges <= legal:
        raise ValueError('Illegal knight move in tour')
    return edges


def distances(edges, starts):
    neighbors = {}
    for a, b in edges:
        neighbors.setdefault(a, []).append(b)
        neighbors.setdefault(b, []).append(a)
    distance = {s: 0 for s in starts}
    queue = deque(starts)
    while queue:
        v = queue.popleft()
        for u in neighbors.get(v, []):
            if u not in distance:
                distance[u] = distance[v] + 1
                queue.append(u)
    return distance


def repairs(cost, squares, surviving, new_edges):
    for removed in combinations(surviving, cost):
        kept = surviving.difference(removed)
        degrees = Counter(v for edge in kept for v in edge)
        deficit = {v: 2 - degrees[v] for v in squares if degrees[v] < 2}
        choices = [e for e in combinations(sorted(deficit), 2) if e in new_edges]
        needed = sum(deficit.values()) // 2
        for added in combinations(choices, needed):
            degrees_added = Counter(v for edge in added for v in edge)
            if degrees_added == deficit:
                candidate = kept.union(added)
                if len(distances(candidate, [min(squares)])) == len(squares):
                    yield candidate


def main():
    data = json.loads((Path(__file__).resolve().parent.parent / 'certificates' / 'counterexample.json').read_text())
    side = data['side']
    deleted = set(data['deleted'])
    if side != 10 or deleted != {0, 1}:
        raise ValueError('Expected the 10-by-10 board with squares 0 and 1 deleted')
    board = set(range(side * side))
    squares = board - deleted
    legal = {e for e in combinations(sorted(board), 2) if knight_move(*e, side)}
    original = tour_edges(data['original_tour'], board, legal)
    witness = tour_edges(data['repaired_tour'], squares, legal)
    surviving = {e for e in original if deleted.isdisjoint(e)}
    new_edges = {e for e in legal - surviving if deleted.isdisjoint(e)}

    for cost in (0, 1, 2):
        found = list(repairs(cost, squares, surviving, new_edges))
        print(f'Cost {cost}: {len(found)} Hamiltonian repairs')
        expected = [witness] if cost == 2 else []
        if found != expected:
            raise ValueError(f'Certificate failed at cost {cost}')

    distance = distances(legal, deleted)
    changed = surviving.symmetric_difference(witness)
    radius = max(distance[v] for edge in changed for v in edge)
    if radius != 6:
        raise ValueError(f'Unexpected radius: {radius}')
    print(f'Unique minimum-cost repair: cost 2, radius {radius} > {side // 2}.')


if __name__ == '__main__':
    main()
