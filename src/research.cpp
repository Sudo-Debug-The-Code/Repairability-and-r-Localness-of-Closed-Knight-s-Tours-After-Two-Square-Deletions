#include <bits/stdc++.h>
#ifndef GENERATION_ONLY
#include "gurobi_c++.h"
#endif
#include <filesystem>

using namespace std;

#include "tour_generator.h"
#include "common.h"
#include "repair_optimizer.h"

int main(int argc,char* argv[]){
    locale::global(locale::classic());
    unsigned long long seedInput = argc > 1 ? stoull(argv[1]) : random_device{}();
    int targetTours = 100;
    int firstBoard = 6;
    int lastBoard = 14;
    int threads = 1;
    string mode = "repair";

    if(argc > 2) targetTours = stoi(argv[2]);
    if(argc > 3) firstBoard = stoi(argv[3]);
    if(argc > 4) lastBoard = stoi(argv[4]);
    if(argc > 5) threads = stoi(argv[5]);
    if(argc > 6) mode = argv[6];

    if(seedInput > UINT32_MAX){
        cerr << "Seed must fit in 32 bits.\n";
        return 1;
    }
    unsigned int seed = seedInput;
#ifdef GENERATION_ONLY
    if(argc<=6) mode="generate";
    if(mode != "generate"){
        cerr << "This build supports generation only.\n";
        return 1;
    }
#endif
    if(targetTours < 1 || firstBoard < 6 || lastBoard < firstBoard || firstBoard % 2 || lastBoard % 2 ||
       lastBoard > 100 || threads < 1 || (mode != "repair" && mode != "generate") || argc > 7){
        cerr << "Usage: research [seed] [target] [first n] [last n] [threads] [repair|generate]\n";
        return 1;
    }
    auto begin=chrono::steady_clock::now();
    auto stamp=chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
    string folder="data/run_"+to_string(stamp)+"_seed_"+to_string(seed);
    if(!filesystem::create_directories(folder)){
        cerr << "Cannot create run directory.\n";
        return 1;
    }
    ofstream info(folder+"/run_info.txt"), generation(folder+"/generation_statistics.csv");
    CheckFile(info); CheckFile(generation);
    info<<setprecision(17)<<"Format version: 2\nSeed: "<<seed<<"\nRNG: std::mt19937; one continuous stream across board sizes\n"
        <<"Target unique tours: "<<targetTours<<"\nBoard sizes: "<<firstBoard<<" to "<<lastBoard<<" step 2\n"
        <<"Mode: "<<mode<<"\nMaximum attempts per board: "<<MAX_ATTEMPTS<<"\nBacktracking node cutoff: "<<NODE_LIMIT
        <<"\nWarnsdorff probability: "<<WARNSDORFF_PROBABILITY<<"\nParameter tuning: not recorded; document separately\n"
        <<"Vertex encoding: zero-based row*n+column; cycles close implicitly\n"
        <<"Objective: removed surviving edges, then minimum locality radius at fixed cost\n"
        <<"Symmetry: cyclic rotation, reversal, eight board symmetries\n"
        <<"Compiler: "<<__VERSION__<<"\nC++ standard: "<<__cplusplus<<"\n";
#ifdef _WIN32
    info<<"Build platform: Windows\n";
#else
    info<<"Build platform: non-Windows\n";
#endif
    if(filesystem::is_regular_file("hardware.txt")){
        filesystem::copy_file("hardware.txt",folder+"/hardware.txt");
        info<<"Hardware details: hardware.txt (provided by operator)\n";
    }
    info<<"Hardware concurrency: "<<thread::hardware_concurrency()<<"\nCommand:";
    for(int i=0;i<argc;i++) info<<' '<<argv[i];
    info<<"\n"; info.flush(); CheckFile(info);
    generation<<"board_size,attempts,closed_before_dedup,failed_attempts,cutoff_failures,duplicate_closed_tours,unique_tours,target_reached,acceptance_rate,dedup_retention_rate,generation_seconds\n";
    generation<<setprecision(17);
    cout<<"Seed: "<<seed<<"\nOutput: "<<folder<<endl;
#ifndef GENERATION_ONLY
    GRBEnv env(true);
    ofstream summary, radiusFile;
    if(mode=="repair"){
        env.set(GRB_IntParam_OutputFlag,0);
        env.set(GRB_IntParam_Threads,threads);
        env.set(GRB_IntParam_Seed,0);
        env.set(GRB_DoubleParam_MIPGap,0.0);
        env.set(GRB_DoubleParam_MIPGapAbs,0.0);
        env.set(GRB_DoubleParam_TimeLimit,GRB_INFINITY);
        env.set(GRB_IntParam_LazyConstraints,1);
        env.set(GRB_IntParam_DualReductions,0);
        env.start();
        info<<"Gurobi version: "<<GRB_VERSION_MAJOR<<'.'<<GRB_VERSION_MINOR<<'.'<<GRB_VERSION_TECHNICAL<<'\n'
            <<"Solver threads: "<<env.get(GRB_IntParam_Threads)<<"\nSolver seed: "<<env.get(GRB_IntParam_Seed)
            <<"\nMIPGap: "<<env.get(GRB_DoubleParam_MIPGap)<<"\nMIPGapAbs: "<<env.get(GRB_DoubleParam_MIPGapAbs)
            <<"\nFeasibilityTol: "<<env.get(GRB_DoubleParam_FeasibilityTol)<<"\nIntFeasTol: "<<env.get(GRB_DoubleParam_IntFeasTol)
            <<"\nOptimalityTol: "<<env.get(GRB_DoubleParam_OptimalityTol)<<"\nTimeLimit: "<<env.get(GRB_DoubleParam_TimeLimit)
            <<"\nLazyConstraints: "<<env.get(GRB_IntParam_LazyConstraints)<<"\nDualReductions: "<<env.get(GRB_IntParam_DualReductions)<<"\n";
        env.writeParams(folder+"/gurobi.prm");
        info.flush(); CheckFile(info);
        summary.open(folder+"/summary.csv"); radiusFile.open(folder+"/radius_counts.csv");
        CheckFile(summary); CheckFile(radiusFile);
        summary<<"board_size,unique_tours,deletion_pairs,repairable_pairs_proven,nonrepairable_pairs_proven,unknown_pairs,expected_cases,optimal,filtered_out,infeasible,unresolved,minimum_cost,maximum_cost,total_cost,average_cost,seconds\n"<<setprecision(17);
        radiusFile<<"board_size,radius,count\n";
    }
#endif
    mt19937 rng(seed);
    for(int n=firstBoard;n<=lastBoard;n+=2){
        auto boardStart=chrono::steady_clock::now();
        auto graph=MakeGraph(n);
        set<vector<int>> uniqueTours;
        long long attempts=0,closedTours=0,cutoffFailures=0;
        while(uniqueTours.size()<(size_t)targetTours && attempts<MAX_ATTEMPTS){
            attempts++;
            long long nodes=0;
            auto candidate=generateRandomTour(graph,rng,nodes);
            if(candidate.empty()){
                if(nodes > NODE_LIMIT) cutoffFailures++;
                continue;
            }
            closedTours++;
            if(!ValidCycle(candidate,graph)){
                cerr << "Invalid generated tour.\n";
                return 1;
            }
            uniqueTours.insert(RemoveBoardSymmetry(candidate,n));
        }
        vector<vector<int>> tours(uniqueTours.begin(),uniqueTours.end());
        long long tourCount = tours.size();
        double generationSeconds=chrono::duration<double>(chrono::steady_clock::now()-boardStart).count();
        generation<<n<<','<<attempts<<','<<closedTours<<','<<attempts-closedTours<<','<<cutoffFailures<<','<<closedTours-tourCount<<','<<tourCount<<','<<(tourCount==(size_t)targetTours)<<','
                  <<CsvNumber(attempts?double(closedTours)/attempts:NAN)<<','<<CsvNumber(closedTours?double(tourCount)/closedTours:NAN)<<','<<generationSeconds<<'\n';
        generation.flush(); CheckFile(generation);
        ofstream tourFile(folder+"/tours_n"+to_string(n)+".csv");
        tourFile<<"board_size,tour_id,original_cycle\n";
        for(size_t i=0;i<tourCount;i++) tourFile<<n<<','<<i+1<<','<<SerializeCycle(tours[i])<<'\n';
        tourFile.close(); CheckFile(tourFile);
        cout<<"n="<<n<<" attempts="<<attempts<<" closed="<<closedTours<<" unique="<<tourCount<<endl;
        if(tourCount!=(size_t)targetTours) cout<<"WARNING: target not reached; actual sample size will be recorded."<<endl;
        if(mode=="generate") continue;
#ifndef GENERATION_ONLY
        ofstream pairs(folder+"/pairs_n"+to_string(n)+".csv"), repairs;
        pairs<<"board_size,deleted_a,deleted_b,status,filter_reason,optimal_cases,infeasible_cases,unresolved_cases\n";
        long long rows=0,part=0,optimal=0,filtered=0,infeasible=0,unresolved=0,totalCost=0;
        long long yesPairs=0,noPairs=0,unknownPairs=0;
        int minCost=INT_MAX,maxCost=0;
        vector<long long> radiusCounts(n*n+1,0);
        auto writeRepair=[&](size_t id,int a,int b,const RepairResult& r){
            if(rows%100000==0){
                if(repairs.is_open()){
                    repairs.close();
                    CheckFile(repairs);
                }
                repairs.open(folder+"/repairs_n"+to_string(n)+"_part"+to_string(++part)+".csv"); CheckFile(repairs);
                repairs<<"board_size,tour_id,deleted_a,deleted_b,status,reason,cost_status,cost_objective,cost_bound,cost_seconds,cost_cuts,radius_status,radius_objective,radius_bound,radius_seconds,radius_cuts,repair_cost,new_edges,symmetric_difference_cost,radius,repaired_cycle\n";
            }
            repairs<<n<<','<<id<<','<<a<<','<<b<<','<<r.status<<','<<CsvText(r.reason)<<',';
            WriteStage(repairs,r.costStage);repairs<<',';WriteStage(repairs,r.radiusStage);
            repairs<<','<<CsvInt(r.cost)<<','<<CsvInt(r.newEdges)<<','<<CsvInt(r.cost<0?-1:r.cost+r.newEdges)<<','<<CsvInt(r.radius)<<','<<SerializeCycle(r.cycle)<<'\n';
            rows++;
        };
        vector<vector<bool>> weights(n*n,vector<bool>(n*n,true));
        for(int a=0;a<n*n;a++){
            for(int b=a+1;b<n*n;b++){
                string failure=HamiltonicityFailure(a,b,n,graph);
                long long pairOptimal=0,pairInfeasible=0,pairUnresolved=0;
                bool provedFeasible=false;
                for(size_t i=0;i<tourCount;i++){
                    RepairResult r;
                    if(!failure.empty()){
                        r.status = "filtered_out";
                        r.reason = failure;
                        filtered++;
                    }
                    else{
                        AssignWeights(n,tours[i],weights,0);
                        r = RepairTourILP(env,n,a,b,tours[i],graph,weights);
                        AssignWeights(n,tours[i],weights,1);
                        if(r.status=="optimal") provedFeasible=true;
                        if(r.status=="optimal"){
                            pairOptimal++;
                            optimal++;
                            minCost = min(minCost,r.cost);
                            maxCost = max(maxCost,r.cost);
                            totalCost += r.cost;
                            radiusCounts.at(r.radius)++;
                        }
                        else if(r.status == "infeasible"){
                            pairInfeasible++;
                            infeasible++;
                        }
                        else{
                            pairUnresolved++;
                            unresolved++;
                        }
                    }
                    writeRepair(i+1,a,b,r);
                }
                string pairStatus;
                if(provedFeasible && pairInfeasible > 0){
                    cerr << "Contradictory solver results.\n";
                    return 1;
                }
                if(!failure.empty() || pairInfeasible > 0){
                    pairStatus = "nonrepairable";
                    noPairs++;
                }
                else if(provedFeasible){
                    pairStatus = "repairable";
                    yesPairs++;
                }
                else{
                    pairStatus = "unknown";
                    unknownPairs++;
                }
                pairs<<n<<','<<a<<','<<b<<','<<pairStatus<<','<<failure<<','<<pairOptimal<<','<<pairInfeasible<<','<<pairUnresolved<<'\n';
                if(repairs.is_open()){repairs.flush();CheckFile(repairs);}
            }
            pairs.flush();CheckFile(pairs);
            cout<<"n="<<n<<" processed "<<a+1<<'/'<<n*n<<endl;
        }
        if(repairs.is_open()){repairs.close();CheckFile(repairs);}
        pairs.close();CheckFile(pairs);
        long long allPairs=1LL*n*n*(n*n-1)/2,expected=allPairs*tourCount;
        if(rows != expected || rows != optimal + filtered + infeasible + unresolved ||
           yesPairs + noPairs + unknownPairs != allPairs){
            cerr << "Case count mismatch.\n";
            return 1;
        }
        for(size_t r=0;r<radiusCounts.size();r++) radiusFile<<n<<','<<r<<','<<radiusCounts[r]<<'\n';
        summary<<n<<','<<tourCount<<','<<allPairs<<','<<yesPairs<<','<<noPairs<<','<<unknownPairs<<','<<expected<<','<<optimal<<','<<filtered<<','<<infeasible<<','<<unresolved<<','
               <<(optimal?to_string(minCost):"")<<','<<(optimal?to_string(maxCost):"")<<','<<totalCost<<','<<CsvNumber(optimal?double(totalCost)/optimal:NAN)<<','
               <<chrono::duration<double>(chrono::steady_clock::now()-boardStart).count()<<'\n';
        summary.flush(); radiusFile.flush(); CheckFile(summary);CheckFile(radiusFile);
        cout<<"Optimal: "<<optimal<<" unresolved: "<<unresolved<<" repairable pairs: "<<yesPairs<<endl;
#endif
    }
    info<<"Elapsed seconds: "<<chrono::duration<double>(chrono::steady_clock::now()-begin).count()<<"\nRun complete: 1\n";
    info.flush();CheckFile(info);
    cout<<"Finished: "<<folder<<endl;
    return 0;
}
