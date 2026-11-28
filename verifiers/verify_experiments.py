import argparse
import csv
import math
from collections import Counter, deque
from pathlib import Path
from itertools import combinations


def require(condition, message):
    if not condition:
        raise ValueError(message)


def rows(path):
    with path.open(newline='', encoding='utf-8-sig') as f:
        yield from csv.DictReader(f)


def edge_set(cycle, n, deleted=()):
    require(len(cycle) == n*n-len(deleted) and set(cycle) == set(range(n*n))-set(deleted), 'Invalid vertex coverage')
    edges = set()
    for u, v in zip(cycle, cycle[1:]+cycle[:1]):
        r, c = divmod(u, n)
        s, t = divmod(v, n)
        require(sorted((abs(r-s), abs(c-t))) == [1,2], 'Invalid knight move')
        edges.add(tuple(sorted((u,v))))
    return edges


def canonical(cycle, n):
    variants = []
    for k in range(8):
        seq = []
        for v in cycle:
            r,c = divmod(v,n)
            x,y = [(r,c),(c,n-1-r),(n-1-r,n-1-c),(n-1-c,r),(n-1-r,c),(r,n-1-c),(c,r),(n-1-c,n-1-r)][k]
            seq.append(x*n+y)
        for order in (seq, seq[::-1]):
            p = order.index(0)
            variants.append(tuple(order[p:]+order[:p]))
    return min(variants)


def graph(n):
    return [[v for v in range(n*n) if sorted((abs(u//n-v//n),abs(u%n-v%n))) == [1,2]] for u in range(n*n)]


def distances(g,a,b):
    d = {a:0,b:0}
    q = deque([a,b])
    while q:
        u=q.popleft()
        for v in g[u]:
            if v not in d:
                d[v]=d[u]+1
                q.append(v)
    return d


def filter_reason(g, n, a, b):
    color = lambda v: (v//n + v%n) % 2
    if color(a) == color(b):
        return 'same_color'
    alive = set(range(n*n)) - {a,b}
    adj = {v: [u for u in g[v] if u in alive] for v in alive}
    if any(len(adj[v]) < 2 for v in alive):
        return 'degree_below_two'
    seen = set()
    def visit(v, parent):
        seen.add(v)
        disc[v] = low[v] = len(seen)
        children = 0
        cut = False
        for u in adj[v]:
            if u == parent:
                continue
            if u in seen:
                low[v] = min(low[v], disc[u])
            else:
                children += 1
                child_cut = visit(u, v)
                cut = cut or child_cut or (parent is not None and low[u] >= disc[v])
                low[v] = min(low[v], low[u])
        return cut or (parent is None and children > 1)
    disc, low = {}, {}
    cut = visit(min(alive), None)
    if seen != alive:
        return 'disconnected'
    if cut:
        return 'articulation_vertex'
    match = {}
    def augment(v, seen_white):
        for u in adj[v]:
            if u in seen_white:
                continue
            seen_white.add(u)
            if u not in match or augment(match[u], seen_white):
                match[u] = v
                return True
        return False
    for v in sorted(alive):
        if color(v) == 0 and not augment(v, set()):
            return 'no_perfect_matching'
    return ''


def run(folder):
    info = (folder/'run_info.txt').read_text(encoding='utf-8')
    require('Run complete: 1' in info, 'Run did not complete')
    metadata = dict(line.split(': ', 1) for line in info.splitlines() if ': ' in line)
    require(metadata.get('Format version') == '2', 'Unexpected data format')
    mode = metadata.get('Mode')
    require(mode in ('generate','repair'), 'Missing or invalid run mode')
    first, _, last, _, step = metadata['Board sizes'].split()
    boards = list(range(int(first),int(last)+1,int(step)))
    target = int(metadata['Target unique tours'])
    gen = list(rows(folder/'generation_statistics.csv'))
    require([int(r['board_size']) for r in gen] == boards, 'Missing or duplicate board in generation statistics')
    summary, radii = {}, {}
    if mode == 'repair':
        summary_rows = list(rows(folder/'summary.csv'))
        require([int(r['board_size']) for r in summary_rows] == boards, 'Missing or duplicate summary board')
        summary = {int(r['board_size']):r for r in summary_rows}
        for row in rows(folder/'radius_counts.csv'):
            k = int(row['board_size']), int(row['radius'])
            require(k not in radii, 'Duplicate histogram entry')
            radii[k] = int(row['count'])
        require(set(radii) == {(n,r) for n in boards for r in range(n*n+1)}, 'Histogram coverage mismatch')
    for r in gen:
        n = int(r['board_size'])
        A,C,U = (int(r[k]) for k in ('attempts','closed_before_dedup','unique_tours'))
        require(0<=U<=C<=A and U<=target and A<=int(metadata['Maximum attempts per board']), 'Invalid generation counts')
        require(int(r['target_reached'])==int(U==target), 'Incorrect target flag')
        if U<target: require(A==int(metadata['Maximum attempts per board']), 'Generation ended before target or attempt limit')
        require(A-C==int(r['failed_attempts']) and C-U==int(r['duplicate_closed_tours']), 'Generation count mismatch')
        require(0<=int(r['cutoff_failures'])<=A-C, 'Invalid cutoff count')
        if A: require(math.isclose(float(r['acceptance_rate']),C/A), 'Acceptance rate mismatch')
        if C: require(math.isclose(float(r['dedup_retention_rate']),U/C), 'Retention rate mismatch')
        originals, seen = {}, set()
        for t in rows(folder/f'tours_n{n}.csv'):
            i=int(t['tour_id']); cycle=list(map(int,t['original_cycle'].split()))
            require(int(t['board_size'])==n and i not in originals, 'Invalid tour ID')
            edges=edge_set(cycle,n)
            canon=canonical(cycle,n)
            require(tuple(cycle)==canon and canon not in seen, 'Duplicate/noncanonical tour')
            seen.add(canon); originals[i]=edges
        require(set(originals)==set(range(1,U+1)), 'Tour count mismatch')
        if mode == 'generate':
            print(f'n={n}: {U} tours verified (generation only)')
            continue
        paths = sorted(folder.glob(f'repairs_n{n}_part*.csv'),key=lambda p:int(p.stem.split('part')[-1]))
        stream = (row for path in paths for row in rows(path))
        g=graph(n); status=Counter(); pair_status=Counter(); hist=Counter(); total=0; lo=None; hi=None
        pstream=iter(rows(folder/f'pairs_n{n}.csv'))
        for a,b in combinations(range(n*n),2):
            d=distances(g,a,b); pc=Counter(); failure=filter_reason(g,n,a,b)
            for i in range(1,U+1):
                t=next(stream,None)
                require(t is not None and tuple(int(t[k]) for k in ('board_size','deleted_a','deleted_b','tour_id'))==(n,a,b,i),'Missing, duplicate or out-of-order repair row')
                st=t['status']; status[st]+=1; pc[st]+=1
                require(st in ('optimal','filtered_out','infeasible','unresolved'),'Unknown status')
                require((st=='filtered_out')==bool(failure), 'Incorrect preliminary filter status')
                if failure: require(t['reason']==failure, 'Incorrect filter reason')
                if st=='optimal':
                    require(t['cost_status']=='2' and t['radius_status']=='2','Optimal row without two optimal statuses')
                    repaired=edge_set(list(map(int,t['repaired_cycle'].split())),n,(a,b))
                    orig={e for e in originals[i] if a not in e and b not in e}
                    removed=orig-repaired; added=repaired-orig
                    rad=max(d[v] for e in removed|added for v in e)
                    cost=len(removed)
                    require((cost,len(added),cost+len(added),rad)==tuple(int(t[k]) for k in ('repair_cost','new_edges','symmetric_difference_cost','radius')),'Repair witness metric mismatch')
                    require(math.isclose(float(t['cost_objective']),cost,abs_tol=1e-5) and math.isclose(float(t['radius_objective']),rad,abs_tol=1e-5),'Objective mismatch')
                    require(math.isclose(float(t['cost_bound']),cost,rel_tol=0,abs_tol=1e-5) and math.isclose(float(t['radius_bound']),rad,rel_tol=0,abs_tol=1e-5),'Recorded solver bound mismatch')
                    hist[rad]+=1; total+=cost;lo=cost if lo is None else min(lo,cost);hi=cost if hi is None else max(hi,cost)
                elif st=='infeasible':
                    require(t['cost_status']=='3','Infeasible row without infeasible solver status')
            p=next(pstream,None)
            require(p is not None and tuple(int(p[k]) for k in ('board_size','deleted_a','deleted_b'))==(n,a,b),'Pair coverage mismatch')
            require(p['filter_reason']==failure,'Pair filter reason mismatch')
            require([int(p[k]) for k in ('optimal_cases','infeasible_cases','unresolved_cases')]==[pc[k] for k in ('optimal','infeasible','unresolved')],'Pair status counts mismatch')
            require(not (pc['optimal'] and pc['infeasible']),'Contradictory pair statuses')
            expected='nonrepairable' if p['filter_reason'] or pc['infeasible'] else 'repairable' if pc['optimal'] else 'unknown'
            require(p['status']==expected,'Pair classification mismatch')
            pair_status[expected]+=1
        require(next(stream,None) is None and next(pstream,None) is None,'Unexpected extra records')
        s=summary[n]
        checks={'unique_tours':U,'deletion_pairs':n*n*(n*n-1)//2,'expected_cases':sum(status.values()),'total_cost':total,'repairable_pairs_proven':pair_status['repairable'],'nonrepairable_pairs_proven':pair_status['nonrepairable'],'unknown_pairs':pair_status['unknown'],**{k:status[k] for k in ('optimal','filtered_out','infeasible','unresolved')}}
        require(all(int(s[k])==v for k,v in checks.items()),'Summary count mismatch')
        if status['optimal']:
            require(int(s['minimum_cost'])==lo and int(s['maximum_cost'])==hi and math.isclose(float(s['average_cost']),total/status['optimal']),'Cost summary mismatch')
        require(all(radii[n,r]==hist[r] for r in range(n*n+1)),'Radius histogram mismatch')
        print(f'n={n}: {U} tours, {sum(status.values())} cases, {status["optimal"]} valid repair witnesses, {status["unresolved"]} unresolved')
    print('Verification passed. This checks witnesses, preliminary filters and accounting; recorded solver statuses and bounds are not independent optimality or general infeasibility proofs.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('folder',type=Path)
    run(p.parse_args().folder)