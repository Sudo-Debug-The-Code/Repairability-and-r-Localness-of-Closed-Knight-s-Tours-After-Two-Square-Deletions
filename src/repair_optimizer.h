// Connectivity Check
bool IsConnected(
    int a,
    int b,
    const vector<vector<int>>& AdjacencyList
){
    int N = AdjacencyList.size();

    vector<bool> visited(N, false);
    queue<int> q;

    int start = -1;

    for(int i = 0; i < N; i++){
        if(i != a && i != b){
            start = i;
            break;
        }
    }

    if(start == -1) return false;

    visited[start] = true;
    q.push(start);

    while(!q.empty()){
        int current = q.front();
        q.pop();

        for(int next : AdjacencyList[current]){
            if(next == a || next == b || visited[next]){
                continue;
            }

            visited[next] = true;
            q.push(next);
        }
    }

    for(int i = 0; i < N; i++){
        if(i != a && i != b && !visited[i]){
            return false;
        }
    }

    return true;
}


// DFS for Articulation Vertices
void ArticulationDFS(
    int current,
    int parent,
    int a,
    int b,
    const vector<vector<int>>& AdjacencyList,
    vector<bool>& visited,
    vector<int>& discovery,
    vector<int>& low,
    int& timer,
    bool& hasArticulation
){
    visited[current] = true;
    discovery[current] = low[current] = timer++;

    int children = 0;

    for(int next : AdjacencyList[current]){
        if(next == a || next == b) continue;

        if(!visited[next]){
            children++;

            ArticulationDFS(
                next, current, a, b,
                AdjacencyList, visited,
                discovery, low, timer,
                hasArticulation
            );

            low[current] = min(low[current], low[next]);

            if(parent != -1 &&
               low[next] >= discovery[current]){
                hasArticulation = true;
            }
        }
        else if(next != parent){
            low[current] = min(
                low[current], discovery[next]
            );
        }
    }

    if(parent == -1 && children > 1){
        hasArticulation = true;
    }
}


// Articulation Vertex Check
bool HasArticulationVertex(
    int a,
    int b,
    const vector<vector<int>>& AdjacencyList
){
    int N = AdjacencyList.size();

    vector<bool> visited(N, false);
    vector<int> discovery(N, -1);
    vector<int> low(N, -1);

    int timer = 0;
    bool hasArticulation = false;

    int start = -1;

    for(int i = 0; i < N; i++){
        if(i != a && i != b){
            start = i;
            break;
        }
    }

    if(start == -1) return true;

    ArticulationDFS(
        start, -1, a, b,
        AdjacencyList, visited,
        discovery, low, timer,
        hasArticulation
    );

    return hasArticulation;
}


// Kuhn DFS for Bipartite Matching
bool MatchingDFS(
    int current,
    int a,
    int b,
    int n,
    const vector<vector<int>>& AdjacencyList,
    vector<bool>& seen,
    vector<int>& match
){
    for(int next : AdjacencyList[current]){
        if(next == a || next == b || seen[next]){
            continue;
        }

        int row = next / n;
        int col = next % n;

        if((row + col) % 2 == 0) continue;

        seen[next] = true;

        if(match[next] == -1 ||
           MatchingDFS(
               match[next], a, b, n,
               AdjacencyList, seen, match
           )){
            match[next] = current;
            return true;
        }
    }

    return false;
}


// Perfect Matching Check
bool HasPerfectMatching(
    int a,
    int b,
    int n,
    const vector<vector<int>>& AdjacencyList
){
    int N = n * n;

    vector<int> match(N, -1);

    int matchingSize = 0;
    int blackVertices = 0;

    for(int v = 0; v < N; v++){
        if(v == a || v == b) continue;

        int row = v / n;
        int col = v % n;

        if((row + col) % 2 != 0) continue;

        blackVertices++;

        vector<bool> seen(N, false);

        if(MatchingDFS(
            v, a, b, n,
            AdjacencyList, seen, match
        )){
            matchingSize++;
        }
    }

    return matchingSize == blackVertices;
}


// Necessary Hamiltonicity Conditions
string HamiltonicityFailure(
    int a,
    int b,
    int n,
    const vector<vector<int>>& AdjacencyList
){
    int rowA = a / n;
    int colA = a % n;

    int rowB = b / n;
    int colB = b % n;

    int colorA = (rowA + colA) % 2;
    int colorB = (rowB + colB) % 2;

    // Color obstruction
    if(colorA == colorB) return "same_color";

    // Minimum degree obstruction
    for(int v = 0; v < n * n; v++){
        if(v == a || v == b) continue;

        int degree = 0;

        for(int next : AdjacencyList[v]){
            if(next != a && next != b){
                degree++;
            }
        }

        if(degree < 2) return "degree_below_two";
    }

    if(!IsConnected(a, b, AdjacencyList)){
        return "disconnected";
    }

    if(HasArticulationVertex(a, b, AdjacencyList)){
        return "articulation_vertex";
    }

    if(!HasPerfectMatching(a, b, n, AdjacencyList)){
        return "no_perfect_matching";
    }

    return "";
}


void AssignWeights(
    int n,
    const vector<int>& tour,
    vector<vector<bool>>& weights,
    int val
){
    for(int i = 0; i < n*n; i++){
        int u = tour[i];
        int v = tour[(i + 1) % (n*n)];

        weights[u][v] = val;
        weights[v][u] = val;
    }
}


long long EdgeKey(int u, int v){
    if(u > v) swap(u, v);

    return ((long long)u << 32) | (unsigned int)v;
}


// Calculate Locality Radius
int FindRadius(
    int a,
    int b,
    const vector<int>& tour,
    const vector<pair<int, int>>& repairedEdges,
    const vector<vector<int>>& AdjacencyList
){
    int N = AdjacencyList.size();

    unordered_set<long long> originalSurvivingEdges;
    unordered_set<long long> selectedEdges;

    for(int i = 0; i < (int)tour.size(); i++){
        int u = tour[i];
        int v = tour[(i + 1) % tour.size()];

        if(u == a || u == b || v == a || v == b){
            continue;
        }

        originalSurvivingEdges.insert(EdgeKey(u, v));
    }

    for(auto [u, v] : repairedEdges){
        selectedEdges.insert(EdgeKey(u, v));
    }

    vector<bool> support(N, false);

    // Removed original edges
    for(int i = 0; i < (int)tour.size(); i++){
        int u = tour[i];
        int v = tour[(i + 1) % tour.size()];

        if(u == a || u == b || v == a || v == b){
            continue;
        }

        if(!selectedEdges.count(EdgeKey(u, v))){
            support[u] = true;
            support[v] = true;
        }
    }

    // Added edges
    for(auto [u, v] : repairedEdges){
        if(!originalSurvivingEdges.count(EdgeKey(u, v))){
            support[u] = true;
            support[v] = true;
        }
    }

    // Multi-source BFS in the original knight graph
    vector<int> distance(N, -1);
    queue<int> q;

    distance[a] = 0;
    distance[b] = 0;

    q.push(a);
    q.push(b);

    while(!q.empty()){
        int current = q.front();
        q.pop();

        for(int next : AdjacencyList[current]){
            if(distance[next] == -1){
                distance[next] = distance[current] + 1;
                q.push(next);
            }
        }
    }

    int radius = 0;

    for(int v = 0; v < N; v++){
        if(support[v]){
            radius = max(radius, distance[v]);
        }
    }

    return radius;
}


#ifndef GENERATION_ONLY
struct Edge {int u,v;};
struct Stage {
    int status=-1;
    double objective=NAN, bound=NAN, seconds=NAN;
    long long cuts=-1;
};
struct RepairResult {
    string status="unresolved", reason;
    Stage costStage, radiusStage;
    int cost=-1, radius=-1, newEdges=-1;
    vector<int> cycle;
};
// Subtour Elimination Callback
class SubtourCallback : public GRBCallback{
private:
    const vector<GRBVar>& variables;
    const vector<Edge>& edges;
    const vector<int>& aliveVertices;

    int vertexCount;

protected:
    void callback() override{
        if(where != GRB_CB_MIPSOL) return;

        try {
            vector<vector<int>> SelectedAdjacency(vertexCount);

            for(int i = 0; i < (int)edges.size(); i++){
                if(getSolution(variables[i]) > 0.5){
                    int u = edges[i].u;
                    int v = edges[i].v;

                    SelectedAdjacency[u].push_back(v);
                    SelectedAdjacency[v].push_back(u);
                }
            }

            vector<bool> visited(vertexCount, false);

            for(int start : aliveVertices){
                if(visited[start]) continue;

                vector<int> component;
                queue<int> q;

                q.push(start);
                visited[start] = true;

                while(!q.empty()){
                    int current = q.front();
                    q.pop();

                    component.push_back(current);

                    for(int next : SelectedAdjacency[current]){
                        if(!visited[next]){
                            visited[next] = true;
                            q.push(next);
                        }
                    }
                }

                if(component.size() == aliveVertices.size()){
                    continue;
                }

                vector<bool> inComponent(vertexCount, false);

                for(int v : component){
                    inComponent[v] = true;
                }

                GRBLinExpr insideEdges = 0;

                for(int i = 0; i < (int)edges.size(); i++){
                    int u = edges[i].u;
                    int v = edges[i].v;

                    if(inComponent[u] && inComponent[v]){
                        insideEdges += variables[i];
                    }
                }

                ++cuts;
                addLazy(
                    insideEdges <= (int)component.size() - 1
                );
            }
        } catch (...) {
            failed = true;
            abort();
        }
    }

public:
    bool failed = false;
    long long cuts = 0;
    SubtourCallback(
        const vector<GRBVar>& variables,
        const vector<Edge>& edges,
        const vector<int>& aliveVertices,
        int vertexCount
    ) :
        variables(variables),
        edges(edges),
        aliveVertices(aliveVertices),
        vertexCount(vertexCount)
    {}
};



Stage ReadStage(GRBModel& model,long long cuts){
    Stage s;
    s.status=model.get(GRB_IntAttr_Status);
    s.seconds=model.get(GRB_DoubleAttr_Runtime);
    s.cuts=cuts;
    if(model.get(GRB_IntAttr_SolCount)>0)
        s.objective=model.get(GRB_DoubleAttr_ObjVal);
    s.bound=model.get(GRB_DoubleAttr_ObjBound);
    return s;
}

// Two-stage lexicographic optimization, as in the manuscript.
RepairResult RepairTourILP(GRBEnv& env,int n,int a,int b,
        const vector<int>& tour,const vector<vector<int>>& graph,
        const vector<vector<bool>>& weights){
    RepairResult result;
    int N=n*n;
    vector<int> alive;
    vector<Edge> edges;
    for(int u=0;u<N;u++) if(u!=a && u!=b){
        alive.push_back(u);
        for(int v:graph[u]) if(v!=a && v!=b && u<v) edges.push_back({u,v});
    }
    GRBModel model(env);
    vector<GRBVar> x;
    vector<vector<int>> incident(N);
    GRBLinExpr cost=0;
    for(int i=0;i<(int)edges.size();i++){
        auto [u,v]=edges[i];
        x.push_back(model.addVar(0,1,0,GRB_BINARY));
        incident[u].push_back(i); incident[v].push_back(i);
        if(!weights[u][v]) cost+=1-x.back();
    }
    for(int v:alive){
        GRBLinExpr degree=0;
        for(int i:incident[v]) degree+=x[i];
        model.addConstr(degree==2);
    }
    model.set(GRB_IntParam_LazyConstraints,1);
    SubtourCallback callback(x,edges,alive,N);
    model.setCallback(&callback);
    model.setObjective(cost,GRB_MINIMIZE);
    model.optimize();
    result.costStage=ReadStage(model,callback.cuts);
    if(callback.failed){result.reason="cost_callback_error";return result;}
    if(result.costStage.status==GRB_INFEASIBLE){result.status="infeasible";return result;}
    if(result.costStage.status!=GRB_OPTIMAL){result.reason="cost_not_optimal";return result;}
    int bestCost=lround(result.costStage.objective);
    if(abs(result.costStage.objective-bestCost)>1e-5){result.reason="noninteger_cost";return result;}

    // Keep the first-stage optimum, then minimize the radius.
    model.addConstr(cost==bestCost);
    vector<int> d=Distances(a,b,graph);
    vector<GRBVar> y(N);
    int maxD=0;
    for(int v:alive){y[v]=model.addVar(0,1,0,GRB_BINARY);maxD=max(maxD,d[v]);}
    GRBVar R=model.addVar(0,maxD,0,GRB_INTEGER);
    for(int i=0;i<(int)edges.size();i++){
        auto [u,v]=edges[i];
        if(weights[u][v]){
            model.addConstr(y[u]>=x[i]); model.addConstr(y[v]>=x[i]);
        }else{
            model.addConstr(y[u]>=1-x[i]); model.addConstr(y[v]>=1-x[i]);
        }
    }
    for(int v:alive) model.addConstr(R>=d[v]*y[v]);
    model.setObjective(GRBLinExpr(R), GRB_MINIMIZE);
    long long firstCuts=callback.cuts;
    model.optimize();
    result.radiusStage=ReadStage(model,callback.cuts-firstCuts);
    if(callback.failed){result.reason="radius_callback_error";return result;}
    if(result.radiusStage.status!=GRB_OPTIMAL){result.reason="radius_not_optimal";return result;}

    vector<vector<int>> selected(N);
    vector<pair<int,int>> repairedEdges;
    unordered_set<long long> chosen;
    int newEdges=0, removed=0;
    for(int i=0;i<(int)edges.size();i++) if(x[i].get(GRB_DoubleAttr_X)>0.5){
        auto [u,v]=edges[i];
        selected[u].push_back(v); selected[v].push_back(u);
        repairedEdges.push_back({u,v}); chosen.insert(EdgeKey(u,v));
        if(weights[u][v]) newEdges++;
    }
    for(size_t i=0;i<tour.size();i++){
        int u=tour[i],v=tour[(i+1)%tour.size()];
        if(u==a || u==b || v==a || v==b) continue;
        if(!chosen.count(EdgeKey(u,v))) removed++;
    }
    int start=alive[0],current=start,previous=-1;
    vector<int> cycle;
    for(size_t i=0;i<alive.size();i++){
        if(selected[current].size()!=2){result.reason="invalid_degree";return result;}
        cycle.push_back(current);
        int next=selected[current][0]==previous?selected[current][1]:selected[current][0];
        previous=current;current=next;
    }
    if(current!=start || !ValidCycle(cycle,graph,a,b)){
        result.reason="invalid_cycle";return result;
    }
    int radius=FindRadius(a,b,tour,repairedEdges,graph);
    if(removed!=bestCost || abs(radius-result.radiusStage.objective)>1e-5){
        result.reason="objective_validation_failed";return result;
    }
    result.cost=removed; result.radius=radius; result.newEdges=newEdges;
    result.cycle=move(cycle); result.status="optimal";
    return result;
}

void WriteStage(ostream& out,const Stage& s){
    out<<CsvInt(s.status)<<','<<CsvNumber(s.objective)<<','<<CsvNumber(s.bound)
       <<','<<CsvNumber(s.seconds)<<','<<CsvInt(s.cuts);
}
#endif

