// individual.cpp

// :^)
#define GPU_OVERDRIVE 0
extern float g_mutation_rate;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "util.h"
#include <png.hpp>
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "individual.h"
static const int ints_per_gene = 4 + 3*2; // r,g,b,a + 3*x,y

Individual::Individual(int img_width, int img_height, int num_genes, float mutation_rate)
{
    //int i = xxx(3);
    create_pbo(&m_pbo,img_width,img_height, NULL);
    m_genome.resize(num_genes * ints_per_gene);
    m_mutation_rate = mutation_rate;
    m_img_width     = img_width;
    m_img_height    = img_height;
    m_num_genes     = num_genes;
    m_active_genes  = num_genes;
    //m_temp_storage.resize(img_width*img_height);
    m_temp_storage = (GLubyte *)malloc(sizeof(GLubyte)*4*img_width*img_height);

    initialize_genome();
}

Individual::~Individual()
{
}

float Individual::get_fitness(GLubyte *goal_data) // xxx
{
    float fitness = 0.0;
#if 0
#if GPU_OVERDRIVE
    NOT_YET_IMPLEMENTED();
#else
    //glBindBuffer(GL_PIXEL_UNPACK_BUFFER_ARB, goal_pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, m_pbo);
    void *goal_memory = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    assert(goal_memory != NULL);
    unsigned int *ptr = (unsigned int *)goal_memory;
    for(int i = 0; i < m_img_width * m_img_height; ++i) {
        m_temp_storage[i] = ptr[i];  // ???
        //printf("g %d %08x %08x\n",i,ptr[i],m_temp_storage[i]);
    }
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
#endif
#else
    //printf("gd:");
    for(unsigned int i = 0; i < m_img_width * m_img_height; ++i) {
        float rd = goal_data[4*i] - m_temp_storage[4*i];
        float gd = goal_data[4*i+1] - m_temp_storage[4*i+1];
        float bd = goal_data[4*i+2] - m_temp_storage[4*i+2];
        float tmp = rd*rd + gd*gd + bd*bd;
        fitness += tmp;
        //printf(" %d=",i);
        //for(int j = 0; j < 4; ++j) {
        //    printf("%02x",goal_data[4*i+j]);
        //}
    }
    //printf("\n");
#endif
    return fitness;
}

// display the image, we need to know where on the framebuffer
// to read back those pixels, though.  maybe there is a fancy way 
// to do this via ogl, but this is sure simple--have the caller tell us.
void Individual::display(int x0,int y0,int w,int h)
{
    static GLdouble planes[4][4] = {
        {1.0,0.0,0.0,0.0},
        {0.0,1.0,0.0,0.0},
        {-1.0,0.0,0.0,m_img_width},
        {0.0,-1.0,0.0,m_img_height},
    };
       
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    // setup clip planes to only draw in our region
    glEnable(GL_CLIP_PLANE0);
    glEnable(GL_CLIP_PLANE1);
    glEnable(GL_CLIP_PLANE2);
    glEnable(GL_CLIP_PLANE3);
    glClipPlane(GL_CLIP_PLANE0,planes[0]);
    glClipPlane(GL_CLIP_PLANE1,planes[1]);
    glClipPlane(GL_CLIP_PLANE2,planes[2]);
    glClipPlane(GL_CLIP_PLANE3,planes[3]);
    // foreach gene, draw color + triangle.
    for(unsigned int i=0; i < m_genome.size(); i += ints_per_gene) {
        int alpha = m_genome[i+3];
        //if(i >= m_active_genes) {
        //    alpha = 0;
        //}
        glColor4ub(m_genome[i+0],m_genome[i+1],m_genome[i+2],alpha);
        glBegin(GL_TRIANGLES); {
            glVertex2i(m_genome[i+4],m_genome[i+5]);
            glVertex2i(m_genome[i+6],m_genome[i+7]);
            glVertex2i(m_genome[i+8],m_genome[i+9]);
        } glEnd();
    }
    glDisable(GL_BLEND);
    glDisable(GL_CLIP_PLANE0);
    glDisable(GL_CLIP_PLANE1);
    glDisable(GL_CLIP_PLANE2);
    glDisable(GL_CLIP_PLANE3);

#if 0
    // now get those pixels into the pbo
    //pbo_unregister_cuda(m_pbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER,m_pbo);
    // BGRA for optimal performance, apparently
    glReadPixels(x0,y0,x1,y1,GL_BGRA,GL_UNSIGNED_BYTE,NULL);

    // get those pixels to the host
    void *goal_memory = glMapBuffer(GL_PIXEL_PACK_BUFFER_ARB, GL_READ_ONLY);
    assert(goal_memory != NULL);
    unsigned int *ptr = (unsigned int *)goal_memory;
    for(int i = 0; i < m_img_width * m_img_height; ++i) {
        m_temp_storage[i] = ptr[i];  // ???
        //printf("gg %d %08x %08x\n",i,ptr[i],m_temp_storage[i]);
    }
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER_ARB);
    glBindBuffer(GL_PIXEL_PACK_BUFFER,0);
    //pbo_register_cuda(m_pbo);
#else
    // xxx BGRA for optimal performance, apparently
    glReadPixels(x0,y0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,m_temp_storage);
    //printf("ts:");
    //for(int i = 0; i < m_img_width * m_img_height; ++i) {
    //    printf(" %d=",i);
    //    for(int j = 0; j < 4; ++j) {
    //        printf("%02x",m_temp_storage[4*i+j]);
    //    }
    //}
    //printf("\n");
#endif
}

void Individual::breed(Individual *mommy, Individual *daddy)
{
    for(unsigned int i=0; i < m_genome.size(); i += ints_per_gene) {
    //for(unsigned int i=m_active_genes+; i < m_genome.size(); i += ints_per_gene) {
        float r = (random() % 10000)/10000.0;
        if(r < g_mutation_rate) {
            set_random_gene(i);
        } else if(r < 0.5) {
            copy_gene(mommy,i);
        } else {
            copy_gene(daddy,i);
        }
    }
    return;
}

void Individual::copy_gene(Individual *ind, int i)
{
    for(int j=0; j < ints_per_gene; ++j) {
        m_genome[i+j] = ind->m_genome[i+j];
    }
}

// 50% border on each side
int border_frame(int size)
{
    int i = random() % (2*size);
    i -= size>>1;
    return i;
}

void Individual::set_random_gene(int i)
{
    // r,g,b,a
    m_genome[i+0] = random() % 255;
    m_genome[i+1] = random() % 255;
    m_genome[i+2] = random() % 255;
    m_genome[i+3] = random() % 255;
    // x, y
    m_genome[i+4] = border_frame(m_img_width);
    m_genome[i+5] = border_frame(m_img_height);
    // x, y
    m_genome[i+6] = border_frame(m_img_width);
    m_genome[i+7] = border_frame(m_img_height);
    // x, y
    m_genome[i+8] = border_frame(m_img_width);
    m_genome[i+9] = border_frame(m_img_height);
}

void Individual::initialize_genome()
{
    for(unsigned int i=0; i < m_genome.size(); i += ints_per_gene) {
#if 1
        set_random_gene(i);
#else
        static int c = 0;
        c++;
        // r,g,b,a
        m_genome[i+0] = 255;
        m_genome[i+1] = c;
        m_genome[i+2] = c;
        m_genome[i+3] = 255;
        // x, y
        m_genome[i+4] = 0;
        m_genome[i+5] = 0;
        // x, y
        m_genome[i+6] = 0;
        m_genome[i+7] = m_img_height;
        // x, y
        m_genome[i+8] = m_img_width;
        m_genome[i+9] = 0;
#endif
    }
}

void Individual::dump()
{
    //print that genome
    printf("Genome\n");
    for(unsigned int i=0; i < m_genome.size(); i += ints_per_gene) {
        // r,g,b,a x,y x,y x,y
        printf("%d: 0x%02x%02x%02x%02x %d,%d %d,%d %d,%d\n", i, 
               m_genome[i+0],
               m_genome[i+1],
               m_genome[i+2],
               m_genome[i+3],
               m_genome[i+4],
               m_genome[i+5],
               m_genome[i+6],
               m_genome[i+7],
               m_genome[i+8],
               m_genome[i+9]);
    }
    png::image<png::rgb_pixel > tmp_png(m_img_width, m_img_height);
    int i = 0;
    for(int y = m_img_height-1; y >= 0; --y) {
        for(int x = 0; x < (int)m_img_width; ++x, ++i) {
            tmp_png.set_pixel(x,y,png::rgb_pixel(m_temp_storage[4*i],
                                                 m_temp_storage[4*i+1],
                                                 m_temp_storage[4*i+2]));
        }
    }
    tmp_png.write("galena_out.png");

}
