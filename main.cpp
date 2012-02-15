// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Galena: Genetic Algorithms breeding images.  
// (Lena is a standard image for the industry)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Genetic Algorithm Pseudocode from Wikipedia:
//
// 1. Choose initial population
// 2. Evaluate the fitness of each individual in the population
// 3. Repeat until termination: (time limit or sufficient fitness achieved)
//    a. Select best-ranking individuals to reproduce
//    b. Breed new generation through crossover and/or mutation
//       (genetic operations) and give birth to offspring
//    c. Evaluate the individual fitnesses of the offspring
//    d. Replace worst ranked part of population with offspring

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#if defined(__APPLE__) || defined(MACOSX)
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <cutil_inline.h>
#include "my_cutil_gl_error.h"
#include <png.hpp>

#include "util.h"
#include "individual.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void main_loop();
bool initialize(int argc, char **argv);
bool initialize_goal_image(int argc, char **argv);
bool initialize_population(int argc, char **argv);
bool initialize_glut(int argc, char **argv);
bool initialize_gl();
void display();
void idle();
void reshape(int w, int h);
float evaluate_fitness();
void breed_new_generation();
void dump_most_fit();

// cuda.cu calls
extern "C" bool initialize_cuda(int argc, char **argv);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
const int BORDER = 10;
int                            g_window_width;
int                            g_window_height;
int                            g_window_id;
png::image< png::rgb_pixel > * g_goal_image;
GLubyte *                      g_goal_image_data;
GLuint                         g_goal_image_pbo;
//GLuint                       g_goal_image_tex;
Individual **                  g_images;
int                            g_population_count;
float                          g_max_generations;
float                          g_cur_generation = 0;
int                            g_num_genes;
float                          g_mutation_rate;
float                          g_fitness_goal;
int                            g_random_seed;
float *                        g_fitness_array;
int *                          g_fitness_index_array;
bool                           g_keep_breeding = true;
int                            g_auto_quit;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int
main(int argc, char **argv)
{
    if(initialize(argc, argv)) {
        glutMainLoop();
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool
initialize(int argc, char **argv)
{
    return initialize_goal_image(argc,argv) &&
        initialize_cuda(1,argv) && // xxx hack cuda init
        initialize_glut(argc,argv) &&
        initialize_population(argc,argv); // population needs GL up-n-running
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// arg[1] --> image file name
bool
initialize_goal_image(int argc, char **argv)
{
    if(argc < 2) {
        fprintf(stderr, "ERROR: need png file. argc=%d\n",argc);
        fflush(stderr);
        return false;
    }
    printf("loading png file: %s\n", argv[1]);
    g_goal_image = new png::image< png::rgb_pixel >(argv[1]);
    printf("  w=%d h=%d w*h=%d\n", (int)(g_goal_image->get_width()), (int)(g_goal_image->get_height()), (int)(g_goal_image->get_width() * g_goal_image->get_height()));
    fflush(stdout);
    g_window_width  = 2*g_goal_image->get_width() + 3*BORDER;
    g_window_height = g_goal_image->get_height() + 2*BORDER;
    g_goal_image_data = (GLubyte*)malloc(sizeof(GLubyte) * g_goal_image->get_width() * g_goal_image->get_height() * 4);
    GLubyte *ptr = g_goal_image_data;
    // y-reversing to match the opengl origin
    for(int y = g_goal_image->get_height()-1; y >= 0; --y) {
        for(uint x = 0; x < g_goal_image->get_width(); ++x) {
            *ptr++ = g_goal_image->get_pixel(x,y).red;
            *ptr++ = g_goal_image->get_pixel(x,y).green;
            *ptr++ = g_goal_image->get_pixel(x,y).blue;
            *ptr++ = 255; // alpha
        }
    }       
    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool
initialize_population(int argc, char **argv)
{
    if(argc < 9) {
        fprintf(stderr, "ERROR: need population count, max_generations, num_genes, mutation_rate, fitness_goal, random_seed auto_quit. argc=%d\n",argc);
        fflush(stderr);
        return false;
    }
    g_population_count = atoi(argv[2]);
    assert(g_population_count > 2);
    g_max_generations  = atof(argv[3]);
    g_num_genes        = atoi(argv[4]);
    g_mutation_rate    = atof(argv[5]);
    g_fitness_goal     = atof(argv[6]);
    g_random_seed      = atoi(argv[7]);
    g_auto_quit        = atoi(argv[8]);
    printf("population count: %d\n", g_population_count);
    printf("max_generations:  %f\n", g_max_generations);
    printf("num_genes:        %d\n", g_num_genes);
    printf("mutation_rate:    %f\n", g_mutation_rate);
    printf("fitness_goal:     %f\n", g_fitness_goal);
    printf("random seed:      %d\n", g_random_seed);
    printf("auto quit:        %d\n", g_auto_quit);
    fflush(stdout);
    srandom(g_random_seed);
    g_images = (Individual **)malloc(sizeof(Individual *)*g_population_count);
    for(int i = 0; i < g_population_count; ++i) {
        g_images[i] = new Individual(g_goal_image->get_width(),
                                     g_goal_image->get_height(),
                                     g_num_genes,
                                     g_mutation_rate);
        printf("#%d\n",i);
        g_images[i]->dump();
    }
    g_fitness_array = (float *)malloc(sizeof(float)*g_population_count);
    g_fitness_index_array = (int *)malloc(sizeof(int)*g_population_count);

    return true;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool
initialize_glut(int argc, char **argv)
{
    // Create GL context                                                        
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowSize(g_window_width,g_window_height);
    g_window_id = glutCreateWindow("Galena");

    if(!initialize_gl()) {
        return false;
    }

    // register callbacks                                                       
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutIdleFunc(idle);

    // create pbo for image XXX should prob be in another initialize_image routine...
    create_pbo(&g_goal_image_pbo, g_goal_image->get_width(), g_goal_image->get_height(), g_goal_image_data);
    //create_texture(&g_goal_image_tex, g_goal_image->get_width(), g_goal_image->get_height(), g_goal_image_data);

    return true;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool
initialize_gl()
{
    // initialize necessary OpenGL extensions                                   
    glewInit();
    if (! glewIsSupported(
            "GL_VERSION_2_0 "
            "GL_ARB_pixel_buffer_object "
            "GL_EXT_framebuffer_object "
            )) {
        fprintf(stderr, "ERROR: Support for necessary OpenGL extensions missing\
.");
        fflush(stderr);
        return false;
    }

    glClearColor(0.0,0.0,0.0,0.0);
    glDisable(GL_DEPTH_TEST);
    glViewport(0,0,g_window_width, g_window_height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0,g_window_width,g_window_height,0,-1,1);

    CUT_CHECK_ERROR_GL();

    return true;
}

void
display_goal_image()
{
#if 0
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, g_goal_image_tex);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glPushMatrix();
    glTranslatef(BORDER,BORDER,0.0);
    glBegin(GL_QUADS); {
        glTexCoord2f(0.0, 0.0);
        glVertex3i(0, 0, 0);

        glTexCoord2f(1.0, 0.0);
        glVertex3f(g_goal_image->get_width(), 0, 0);
        
        glTexCoord2f(1.0, 1.0);
        glVertex3f(g_goal_image->get_width(), g_goal_image->get_height(), 0);

        glTexCoord2f(0.0, 1.0);
        glVertex3f(0, g_goal_image->get_height(), 0);
    } glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
#else
    // send pbo -> framebuffer
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,g_goal_image_pbo);
    glRasterPos2i(BORDER,BORDER+g_goal_image->get_height());
    //glPixelZoom(1.0,-1.0);
    // XXX ABGR is optimal perf?
    glDrawPixels(g_goal_image->get_width(), g_goal_image->get_height(),
                 GL_RGBA, GL_UNSIGNED_BYTE, 0); 
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
#endif
    CUT_CHECK_ERROR_GL();
}

void
display_population_images()
{
    glPushMatrix();
    glTranslatef(2*BORDER + g_goal_image->get_width(), BORDER, 0.0);
    if(g_keep_breeding) {
        for(int i = 0; i < g_population_count; ++i) {
            //g_images[i]->m_active_genes = g_num_genes; // 1 + (unsigned int)(g_cur_generation/50.0);
            // "mini clear" to avoid blending between population
            glColor4ub(0,0,0,255);
            glRecti(0,0,g_goal_image->get_width(),g_goal_image->get_height());
            g_images[i]->display(
                2*BORDER + g_goal_image->get_width(),BORDER,
                g_goal_image->get_width(),g_goal_image->get_height()
                );
        }
    } else {
        g_images[g_fitness_index_array[0]]->display(
            2*BORDER + g_goal_image->get_width(),BORDER,
            g_goal_image->get_width(),g_goal_image->get_height()
            );
    }
    glPopMatrix();
    CUT_CHECK_ERROR_GL();
}

void 
display()
{
    if(!g_keep_breeding && g_auto_quit) {
        glutDestroyWindow(g_window_id);
        exit(0);
    }
    cudaEvent_t start, display, evaluate, breed, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&display);
    cudaEventCreate(&evaluate);
    cudaEventCreate(&breed);
    cudaEventCreate(&stop);
    cudaEventRecord(start,0);
    glClear(GL_COLOR_BUFFER_BIT);
    display_population_images();
    display_goal_image();
    cudaEventRecord(display,0); cudaEventSynchronize(display);
    if(g_keep_breeding) {
        float best_fitness = evaluate_fitness();
        cudaEventRecord(evaluate,0); cudaEventSynchronize(evaluate);
        printf("%.0f,%.0f,%.0f,", 
               g_cur_generation, best_fitness,g_fitness_array[g_population_count-1]);
        if(g_cur_generation < g_max_generations && best_fitness > g_fitness_goal) {
            breed_new_generation();
            ++g_cur_generation;
        } else {
            g_keep_breeding = false;
            glClearColor(0.5,0.5,0.5,1.0);
            printf("\n");
            printf("best fitness = %.0f\n",best_fitness);
            dump_most_fit();
        }
        cudaEventRecord(breed,0); cudaEventSynchronize(breed);
    }
    glutSwapBuffers();
    cudaEventRecord(stop,0);
    cudaEventSynchronize(stop);
    float t1,t2,t3,t4;
    cudaEventElapsedTime(&t1, start, display);
    cudaEventElapsedTime(&t2, display, evaluate);
    cudaEventElapsedTime(&t3, evaluate, breed);
    cudaEventElapsedTime(&t4, breed, stop);
    if(g_keep_breeding) {
        printf("%f,%f,%f,%f\n",t1,t2,t3,t4);
    }
    fflush(stdout);
}

//#define MIN(a,b) ((a<b)?(a):(b))
float
evaluate_fitness()
{
    float fitness = -1;
    for(int i = 0; i < g_population_count; ++i) {
        g_fitness_index_array[i] = i;
        g_fitness_array[i] = g_images[i]->get_fitness(g_goal_image_data);
    }
    // oh horrid bubblesort...
    bool swapped;
    do {
        swapped = false;
        for(int i=0; i<g_population_count-1; ++i) {
            if( g_fitness_array[i] > g_fitness_array[i+1] ) {
                float tmpf = g_fitness_array[i];
                g_fitness_array[i] = g_fitness_array[i+1];
                g_fitness_array[i+1] = tmpf;
                int tmpi = g_fitness_index_array[i];
                g_fitness_index_array[i] = g_fitness_index_array[i+1];
                g_fitness_index_array[i+1] = tmpi;
                swapped = true;
            }
        }
    } while(swapped);
#if 0
    printf("sorted fitness\n");
    for(int i = 0; i < g_population_count; ++i) {
        printf("   %d: %f\n",g_fitness_index_array[i],g_fitness_array[i]);
    }
#endif
    fitness = g_fitness_array[0];
    return fitness;
}

void
breed_new_generation()
{
    for(int i = 0; i < g_population_count; ++i) {
        if(i == g_fitness_index_array[0])
            continue;
        g_images[i]->breed(
            g_images[g_fitness_index_array[i]], // mommy is original.  
            g_images[g_fitness_index_array[0]]  // daddy is best fit
            );
    }
}

void
dump_most_fit()
{
    printf("Here's your best-fit genome: \n");
    g_images[g_fitness_index_array[0]]->dump();
}

void 
idle()
{
    glutPostRedisplay();
}

void reshape(int w, int h)
{
    g_window_width = w;
    g_window_height = h;
}
