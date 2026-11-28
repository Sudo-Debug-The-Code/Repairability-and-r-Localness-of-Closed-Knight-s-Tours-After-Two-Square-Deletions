from itertools import combinations
from pathlib import Path


def knight_move(a, b, width):
    row_a, col_a = divmod(a, width)
    row_b, col_b = divmod(b, width)
    return sorted((abs(row_a - row_b), abs(col_a - col_b))) == [1, 2]


def near_end(row, col, height, cut):
    return min(row, height - 1 - row) <= 2 and cut - 3 <= col <= cut + 2


def has_clean_cut(a, b, height, width):
    r1, c1 = divmod(a, width)
    r2, c2 = divmod(b, width)
    for cut in range(6, width - 5, 2):
        if not (near_end(r1, c1, height, cut) or near_end(r2, c2, height, cut)):
            return True
    for cut in range(6, height - 5, 2):
        if not (near_end(c1, r1, width, cut) or near_end(c2, r2, width, cut)):
            return True
    return False


def symmetries(square, height, width):
    row, col = divmod(square, width)
    positions = [(row, col)]
    if height == width:
        positions.append((col, row))
    for r, c in positions:
        for y in (r, height - 1 - r):
            for x in (c, width - 1 - c):
                yield y * width + x


def required_cases():
    for height in range(6, 23, 2):
        for width in range(height, 23, 2):
            squares = range(height * width)
            corners = (0, width - 1, (height - 1) * width, height * width - 1)
            colors = [sum(divmod(v, width)) % 2 for v in squares]
            adjacent_corners = [{c for c in corners if knight_move(c, v, width)}
                                for v in squares]
            images = [list(symmetries(v, height, width)) for v in squares]
            for a, b in combinations(squares, 2):
                if colors[a] == colors[b]:
                    continue
                if adjacent_corners[a] - {b} or adjacent_corners[b] - {a}:
                    continue
                if has_clean_cut(a, b, height, width):
                    continue
                equivalent_pairs = zip(images[a], images[b])
                if [a, b] == min(sorted(pair) for pair in equivalent_pairs):
                    yield height, width, a, b


def check_tour(case, tour):
    height, width, a, b = case
    squares = set(range(height * width)) - {a, b}
    if len(tour) != len(squares) or set(tour) != squares:
        raise ValueError(f'Wrong squares for {case}')
    edges = list(zip(tour, tour[1:] + tour[:1]))
    if not all(knight_move(x, y, width) for x, y in edges):
        raise ValueError(f'Illegal knight move for {case}')


def main():
    missing = set(required_cases())
    count = 0
    with (Path(__file__).resolve().parent.parent / 'certificates' / 'main_theorem_tours.txt').open() as file:
        for line in file:
            if line.startswith('#'):
                continue
            header, sequence = line.split('|')
            case = tuple(map(int, header.split()))
            if case not in missing:
                raise ValueError(f'Duplicate or unexpected case: {case}')
            check_tour(case, list(map(int, sequence.split())))
            missing.remove(case)
            count += 1
    if missing:
        raise ValueError(f'Missing {len(missing)} cases, including {min(missing)}')
    print(f'Main theorem: verified all {count} deletion cases.')


if __name__ == '__main__':
    main()
