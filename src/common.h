// Convert a cycle to a space-separated string.
string SerializeCycle(const vector<int>& cycle){
    string result;

    for(int i = 0; i < (int)cycle.size(); i++){
        if(i > 0) result += " ";

        result += to_string(cycle[i]);
    }

    return result;
}


vector<vector<int>> MakeGraph(int n){
    vector<vector<int>> graph(n*n);
    int dx[] = {1,1,-1,-1,2,2,-2,-2};
    int dy[] = {2,-2,2,-2,1,-1,1,-1};
    for(int r=0;r<n;r++) for(int c=0;c<n;c++)
        for(int k=0;k<8;k++){
            int x=r+dx[k], y=c+dy[k];
            if(x>=0 && x<n && y>=0 && y<n)
                graph[r*n+c].push_back(x*n+y);
        }
    return graph;
}

bool ValidCycle(const vector<int>& cycle, const vector<vector<int>>& graph,
                int a=-1, int b=-1){
    int N=graph.size();
    if(cycle.size()!=N-(a>=0)-(b>=0) || cycle.empty()) return false;
    vector<bool> seen(N,false);
    for(int v:cycle){
        if(v<0 || v>=N || v==a || v==b || seen[v]) return false;
        seen[v]=true;
    }
    for(size_t i=0;i<cycle.size();i++){
        int u=cycle[i], v=cycle[(i+1)%cycle.size()];
        if(find(graph[u].begin(),graph[u].end(),v)==graph[u].end()) return false;
    }
    return true;
}

vector<int> Distances(int a,int b,const vector<vector<int>>& graph){
    vector<int> d(graph.size(),-1);
    queue<int> q;
    d[a]=d[b]=0; q.push(a); q.push(b);
    while(!q.empty()){
        int u=q.front(); q.pop();
        for(int v:graph[u]) if(d[v]<0){d[v]=d[u]+1; q.push(v);}
    }
    return d;
}

string CsvNumber(double x){
    if(!isfinite(x) || abs(x)>=1e90) return "";
    ostringstream out; out.imbue(locale::classic());
    out<<setprecision(17)<<x; return out.str();
}
string CsvInt(long long x){return x<0?"":to_string(x);}
string CsvText(string s){
    string out(1, '"');
    for(char c:s){ if(c=='"') out+='"'; if(c=='\n'||c=='\r') c=' '; out+=c; }
    return out+'"';
}

void CheckFile(ostream& out){
    if(!out){
        cerr << "Output file write failed.\n";
        exit(1);
    }
}

