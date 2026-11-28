const long long NODE_LIMIT = 200000;
const long long MAX_ATTEMPTS = 1000000;
const double WARNSDORFF_PROBABILITY = 0.80;

// Randomized Warnsdorff + Backtracking
bool backtrackTour(
    const vector<vector<int>>& AdjacencyList,
    vector<bool>& visited,
    vector<int>& tour,
    int start,
    mt19937& rng,
    long long& nodes,
    long long nodeLimit
) {
    nodes++;

    if(nodes > nodeLimit) return false;

    int n = visited.size();
    int current = tour.back();

    if(tour.size() == n){
        for(int next : AdjacencyList[current]){
            if(next == start) return true;
        }

        return false;
    }

    vector<pair<int, int>> candidates;

    for(int next : AdjacencyList[current]){
        if(visited[next]) continue;

        int degree = 0;

        for(int neighbor : AdjacencyList[next]){
            if(!visited[neighbor]) degree++;
        }

        candidates.push_back({degree, next});
    }

    if(candidates.empty()) return false;

    shuffle(candidates.begin(), candidates.end(), rng);

    uniform_real_distribution<double> chance(0.0, 1.0);

    if(chance(rng) < WARNSDORFF_PROBABILITY){
        stable_sort(
            candidates.begin(),
            candidates.end(),
            [](const pair<int, int>& a, const pair<int, int>& b){
                return a.first < b.first;
            }
        );
    }

    for(auto candidate : candidates){
        int next = candidate.second;

        if(tour.size() == n - 1){
            bool closes = false;

            for(int neighbor : AdjacencyList[next]){
                if(neighbor == start){
                    closes = true;
                    break;
                }
            }

            if(!closes) continue;
        }

        visited[next] = true;
        tour.push_back(next);

        if(backtrackTour(
            AdjacencyList, visited, tour,
            start, rng, nodes, nodeLimit
        )){
            return true;
        }

        tour.pop_back();
        visited[next] = false;
    }

    return false;
}


// Random Tour Generator
vector<int> generateRandomTour(
    const vector<vector<int>>& AdjacencyList,
    mt19937& rng,
    long long& nodes
){
    int n = AdjacencyList.size();

    vector<bool> visited(n, false);
    vector<int> tour;

    uniform_int_distribution<int> startDist(0, n - 1);

    int start = startDist(rng);

    tour.push_back(start);
    visited[start] = true;

    nodes = 0;
    long long nodeLimit = NODE_LIMIT;

    if(!backtrackTour(
        AdjacencyList, visited, tour,
        start, rng, nodes, nodeLimit
    )){
        return {};
    }

    return tour;
}


// Cycle Symmetry Remover
vector<int> RemoveCycleSymmetry(const vector<int>& path){
    int n = path.size();

    int pos = find(path.begin(), path.end(), 0) - path.begin();

    vector<int> forward;
    vector<int> backward;

    for(int i = 0; i < n; i++){
        forward.push_back(path[(pos + i) % n]);
        backward.push_back(path[(pos - i + n) % n]);
    }

    return min(forward, backward);
}


// Board Symmetries
int transformVertex(int v, int n, int type){
    int row = v / n;
    int col = v % n;

    int newRow = row;
    int newCol = col;

    switch(type){
        case 0:
            newRow = row;
            newCol = col;
            break;

        case 1:
            newRow = col;
            newCol = n - 1 - row;
            break;

        case 2:
            newRow = n - 1 - row;
            newCol = n - 1 - col;
            break;

        case 3:
            newRow = n - 1 - col;
            newCol = row;
            break;

        case 4:
            newRow = n - 1 - row;
            newCol = col;
            break;

        case 5:
            newRow = row;
            newCol = n - 1 - col;
            break;

        case 6:
            newRow = col;
            newCol = row;
            break;

        case 7:
            newRow = n - 1 - col;
            newCol = n - 1 - row;
            break;
    }

    return newRow * n + newCol;
}


// Board + Cycle Symmetry Remover
vector<int> RemoveBoardSymmetry(
    const vector<int>& path,
    int boardSize
){
    vector<int> best;
    bool first = true;

    for(int type = 0; type < 8; type++){
        vector<int> transformed;

        for(int v : path){
            transformed.push_back(
                transformVertex(v, boardSize, type)
            );
        }

        vector<int> normalized =
            RemoveCycleSymmetry(transformed);

        if(first || normalized < best){
            best = normalized;
            first = false;
        }
    }

    return best;
}


