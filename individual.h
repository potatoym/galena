#ifndef GALENA_INDIVIDUAL_H
#define GALENA_INDIVIDUAL_H
// individual.h
#include <GL/gl.h>
#include <vector>
class Individual
{

public:
    Individual(int img_width, int img_height, int num_genes, float mutation_rate);
    ~Individual();
    float get_fitness(GLubyte *goal_data); // xxx GLuint goal_pbo);
    void display(int x0, int y0, int x1, int y1);
    void breed(Individual *mommy, Individual *daddy);
    void dump();

//protected: ?
    std::vector< int > m_genome;
    unsigned int m_active_genes;

private:
    void update_pbo();
    void initialize_genome();
    void copy_gene(Individual *ind, int i);
    void set_random_gene(int i);

    unsigned int m_img_width, m_img_height;
    unsigned int m_num_genes;
    GLuint m_pbo;
    float m_mutation_rate;
//    std::vector< unsigned int > m_temp_storage;
    GLubyte *m_temp_storage;

};
#endif
