from itertools import product
from pathlib import Path


def knight_move(a, b, width):
    row_a, col_a = divmod(a, width)
    row_b, col_b = divmod(b, width)
    return sorted((abs(row_a - row_b), abs(col_a - col_b))) == [1, 2]


def main():
    missing = set(product((6, 8, 10), repeat=2))
    count = 0
    with (Path(__file__).resolve().parent.parent / 'certificates' / 'lemma_1_tours.txt').open() as file:
        for line in file:
            if line.startswith('#'):
                continue
            header, sequence = line.split('|')
            height, width = map(int, header.split())
            case = (height, width)
            if case not in missing:
                raise ValueError(f'Duplicate or unexpected board: {case}')

            tour = list(map(int, sequence.split()))
            if len(tour) != height * width or set(tour) != set(range(height * width)):
                raise ValueError(f'Wrong squares on {case}')
            edges = list(zip(tour, tour[1:] + tour[:1]))
            if not all(knight_move(a, b, width) for a, b in edges):
                raise ValueError(f'Illegal knight move on {case}')
            prescribed = {width + 1, 3 * width}
            if not any({a, b} == prescribed for a, b in edges):
                raise ValueError(f'Missing prescribed edge on {case}')
            missing.remove(case)
            count += 1

    if missing:
        raise ValueError(f'Missing boards: {sorted(missing)}')
    print(f'Lemma 1: verified {count} intact boards and their prescribed edges.')


if __name__ == '__main__':
    main()
