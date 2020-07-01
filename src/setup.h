/*
SITH: An R package for visualizing and analyzing a spatial model of intra-tumor heterogeneity
Author: Phillip Nicol
License: GPL-2 
*/


// preprocessing
#include<iostream> 
#include<vector> 
#include<algorithm> 
#include<cmath> 
#include<float.h> 
#include<string> 
#include<Rcpp.h> 


//How often to print to screen? 
#define interval 2000000

//For randomly choosing colors 
#define rgb_ub 0.91
#define rgb_lb 0.09

//Defined types for simulation
//A specie is a unique genotype in the cell population
struct specie {
    int id;
    int count;
    std::vector<int> genotype;
    double d,b;
};

//A cell is specified by its coordinates and specie type
struct cell {
    short int x,y,z;
    int id;
};

//Global variables
double p_max;
std::vector<int> drivers; 
int total_mutations;
int x_dim, y_dim, z_dim; //Size of the lattice (set at the start of simulation to accomodate num of cells)

bool*** init_lattice(void)
{
    bool*** lattice = new bool**[x_dim];
    for(int i = 0; i < x_dim; ++i)
    {
        lattice[i] = new bool*[y_dim];
        for(int j = 0; j < y_dim; ++j)
        {
            lattice[i][j] = new bool[z_dim];
        }
    }
    //Initialize all zeros
    for(int i = 0; i < x_dim; ++i)
    {
        for(int j = 0; j < y_dim; ++j)
        {
            for(int k = 0; k < z_dim; ++k)
            {
                lattice[i][j][k] = 0;
            }
        }
    }
    lattice[x_dim/2][y_dim/2][z_dim/2] = 1;
    return lattice;
}

void trashcan(bool*** lattice)
{
    // deallocate
    for(int i = 0; i < x_dim; ++i)
    {
        for(int j = 0; j < y_dim; ++j)
        {
            delete[] lattice[i][j];
        } 
        delete[] lattice[i];
    }
    delete[] lattice;
}

cell initial_cell(std::vector<specie> &species, double wt_br, double wt_dr)
{
    //Initial cell lies in the center of the lattice
    cell cell;
    cell.x = x_dim/2;
    cell.y = y_dim/2;
    cell.z = z_dim/2;

    //initial specie type has wild type birth and death rates
    specie initial_type;
    initial_type.b = wt_br;
    initial_type.d = wt_dr;
    initial_type.id = 0;
    initial_type.count = 1;
    initial_type.genotype.push_back(0);

    //Save the species type in the vector
    species.push_back(initial_type);

    //Let the cell id point to this id
    cell.id = initial_type.id;
    return cell;
}

int max_mut(std::vector<specie> &species) {
    int max = 0;
    for(int i = 0; i < species.size(); ++i) {
        if(species[i].genotype.size() > max) {
            max = species[i].genotype.size(); 
        }
    }
    return max;
}

void gv_init(const int N, const double wt_br, const double wt_dr, const double u, const double du, const double s) {
    total_mutations = 0; 
    drivers.clear(); 
    p_max = wt_br + wt_dr;

    //error checking 
    if(N < 1) {Rcpp::stop("N must be at least 2.");}
    if(wt_dr > wt_br) {Rcpp::stop("Death rate can not be greater than birth rate.");}
    if((wt_br < 0) || (wt_dr < 0)) {Rcpp::stop("Birth and death rates must be non-negative.");}
    if(u < 0) {Rcpp::stop("u must be non-negative");}
    if(du < 0.0 || du > 1.0) {Rcpp::stop("du must be in [0,1]");}
    if(s < 0) {Rcpp::stop("s must be non-negative");}

    //set lattice dims 
    if(N > 100000000) {
        //very large lattice dims
        x_dim = 2000; y_dim = 2000; z_dim = 2000;
    }
    else if(N > 10000000) {
        //decently large
        x_dim = 1000; y_dim = 1000; z_dim = 1000;
    }
    else {
        //This should handle most cases
        x_dim = 500; y_dim = 500; z_dim = 500;
    }
}

//Store all permutations 
std::vector<std::vector<int> > get_perms(std::vector<int> v) {
    std::vector<std::vector<int> > perms;

    do {
        std::vector<int> p;
        for(int i = 0; i < v.size(); ++i) {
            p.push_back(v[i]);
        }
        perms.push_back(p);
    } while(std::next_permutation(v.begin(), v.end()));

    return perms;
}