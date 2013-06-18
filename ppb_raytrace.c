#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "ppb_raytrace.h"
#include "timer.h"	    // start_timer, stop_timer

//#define DEBUG               /* �ǥХå�������ͭ��̵�� */
#define OUTPUT     "out.ppm"  /* ��̲����Υե�����̾ */
#define IMG_WIDTH  (400)      /* ��̲������� */
#define IMG_HEIGHT (400)      /* ��̲����ι⤵ */

#define THREAD_NUM 2 //�ɲ�

/* RGB�ͤ򥻥å�*/
void set_color(rgb_t* buf, unsigned char r, unsigned char g, unsigned char b) {
    buf->r = r;
    buf->g = g;
    buf->b = b;
    return;
}

/* RGB�ͤ�ñ������ư�������ξ軻 */
void mult_color(rgb_t* b, rgb_t* a, float t) {
    b->r = (char)(t * a->r);
    b->g = (char)(t * a->g);
    b->b = (char)(t * a->b);
    return;
}

/* RGB�ͤβû� */
void add_color(rgb_t* c, rgb_t* a, rgb_t* b) {
    c->r = a->r + b->r;
    c->g = a->g + b->g;
    c->b = a->b + b->b;
    return;
}

/* ��̲������ΰ��RGB�ͤ��������� */
void draw_pixel(rgb_t* buf, rgb_t* color) {
    buf->r = color->r;
    buf->g = color->g;
    buf->b = color->b;
    return;
}

/* �٥����������Ǥ򥻥å� */
void set_vector(vector_t* v, float x, float y, float z) {
    v->x = x;	// �ݥ���v����vector_t�Υ���x��, ��x������
    v->y = y;
    v->z = z;
    return;
}

/* �٥������βû� */
void add_vector(vector_t* c, vector_t* a, vector_t* b) {
    c->x = a->x + b->x;
    c->y = a->y + b->y;
    c->z = a->z + b->z;
    return;
}

/* �٥������γ����Ǥ�ñ������ư�����������礭�������ͤ��ᡢ
   ��̤�٥������Ȥ����֤�*/
void max_vector(vector_t* c, float n) {
    c->x = Max(c->x, n);
    c->y = Max(c->y, n);
    c->z = Max(c->z, n);
    return;    
}

/* �٥������θ��� */
void sub_vector(vector_t* c, vector_t* a, vector_t* b) {
    c->x = a->x - b->x;
    c->y = a->y - b->y;
    c->z = a->z - b->z;
    return;
}

/* �٥������ξ軻 */
void mult_vector(vector_t* b, vector_t* a, float t) {
    b->x = t * a->x;
    b->y = t * a->y;
    b->z = t * a->z;
    return;
}

/* �٥�������ɸ�ಽ */
void norm_vector(vector_t* a) {
    float d = sqrtf(Sq(a->x) + Sq(a->y) + Sq(a->z));
    a->x /= d;
    a->y /= d;
    a->z /= d;
    return;
}

/* �٥����������� */
float innerproduct_vector(vector_t* a, vector_t* b) {
    return (a->x*b->x + a->y*b->y + a->z*b->z);
}

/* ����������������� */
void new_scene(scene_data_t* scene) {
    /* �����Το����濴��Ⱦ�¤����� */
    set_color(&(scene->ball).color, SPHERE_COLOR);	    // SPHERE_COLOR : �����ΰ���֤����Το�
    set_vector(&(scene->ball).center, SPHERE_CENTER);	    // SPHERE_CENTER : �濴��ɸ
    (scene->ball).radius = SPHERE_RAD;			    // SPHERE_RAD : Ⱦ��

    /* ���طʿ��������κ�ɸ�������κ�ɸ������ */
    set_color(&(scene->bgcolor), BG_COLOR);		    // BG_COLOR : �����ΰ���طʿ�
    set_vector(&(scene->viewpoint), VIEW_POINT);	    // VIEW_POINT : �����κ�ɸ
    set_vector(&(scene->light), LIGHT);			    // LIGHT : �����κ�ɸ
    return;
}

/* �����ΰ������ */
void set_workarea(workarea_t* warea, int sx, int ex, int sy, int ey) {
    warea->startx = sx;
    warea->starty = sy;
    warea->endx = ex;
    warea->endy = ey;
    return;
}


//*****************************************************************************
// �ؿ�̾ : init_trace_thread
// ����   : thread_num:����åɿ�, targ*, warea*, img*, scene*, w, h
// ����� : �ʤ�
// ����   : ����ѥ���åɤ��쥤�ȥ졼���󥰤η׻��򤹤뤿��˻��Ѥ���Ƽ�ѥ�᡼��������
//*****************************************************************************
void init_trace_thread (int thread_num, thread_arg_t* targ, workarea_t* warea, image_t* img, scene_data_t* scene, int w, int h) {
	int i;

	for (i = 0; i < thread_num; ++i) {
	    /* �� ����åɤη׻��ϰϤλ��� */
	    set_workarea(&warea[i], 0, w, i*h/thread_num, (i+1)*h/thread_num);	// �ƥ���åɤη׻��ϰ�
	    /* ���쥤�ȥ졼���׻��ѥѥ�᡼�������� */
	    targ[i].img = img;
	    targ[i].scene = scene;
	    targ[i].warea = warea[i];
	}
	return;
}


//*****************************************************************************
// �ؿ�̾ : start_trace_thread
// ����   : thread_num:����åɿ�, tid, targ
// ����� : �ʤ�
// ����   : init_trace_thread �����ꤵ�줿�ѥ�᡼���˴𤤤ƥ���åɤ����
//*****************************************************************************
void start_trace_thread (int thread_num, pthread_t* tid, thread_arg_t* targ) {
	int i;
	for(i = 0; i < thread_num; ++i)
		ptread_create(&tid[i], NULL, (void*)trace_ray_thread, (void*)&targ[i]);
	return;
}


//*****************************************************************************
// �ؿ�̾ : trace_ray_thread 
// ����   : thread_arg_t��¤�Τ˴ޤޤ��ѥ�᡼��:arg
// ����� : �ʤ�
// ����   : trace_ray�ؿ��θƤӽФ�
//*****************************************************************************
void trace_ray_thread (thread_arg_t* arg) {
	image_t* img = arg->img;
	scene_data_t* scene = arg->scene;
	workarea_t* warea = &arg->warea;

	trace_ray(img, scene, warea);
	return;
}


//*****************************************************************************
// �ؿ�̾ : wait_trace_thread
// ����   : thread_num:����åɿ�, tid
// ����� : �ʤ�
// ����   : ����åɽ�λ�Ե��Τ���δؿ���Ƥ�. ���δؿ��ν����������Х쥤�ȥ졼���󥰤η׻��ϴ�λ
//*****************************************************************************
void wait_trace_thread (int thread_num, pthread_t* tid){
	int i;
	for(i = 0; i < thread_num; ++i) {
		pthread_join(tid[i], NULL);
	}
	return;
}


//*****************************************************************************
// �ؿ�̾ : intersect
// ����   : ball:����, viewpoint:����, view:����
// ����� : t�ʻ����ȵ��ΤȤθ�������뤿���ɬ�פȤʤ뷸����
// ����   : ���������Τȸ��뤫�ɤ����ˤĤ��Ƥ�Ƚ��
//*****************************************************************************
float intersect(sphere_t* ball, vector_t* viewpoint, vector_t* view) { 
    float t;
    vector_t vtmp;
    /* ��Ƚ�̼��η׻� */
    sub_vector(&vtmp, viewpoint, &ball->center);    // ��Ƚ����󼡼�
    float b = innerproduct_vector(view, &vtmp);
    float c = innerproduct_vector(&vtmp, &vtmp) - Sq(ball->radius);
    float d = Sq(b) - c;
    /* ���򺹾�����Ƚ�� */
    if(d < 0) return INFINITY;	    // 0��꾮���� = �򺹤��ʤ��Τ�̵������֤�
    float det = sqrtf(d);
    t = -b - det;
    if(t < 0.05f) t = -b + det;	    // Ƚ�̼����ͤ��������������򤹤�
    if(t < 0) return INFINITY;
    return t;			    // �֤���t
}


//*****************************************************************************
// �ؿ�̾ : shading
// ����   : view:����, light:����, n:ˡ��, color:���Το�, s:���ζ�����ɽ������
// ����� : 
// ����   : �����ο������Ť�׻�����
//*****************************************************************************

/* �������ǥ���(�ե����ǥ�) */
rgb_t shading(vector_t* view, vector_t* light, vector_t* n, rgb_t* color, float s) {
    /* ���������ǥ��󥰥ѥ�᡼������ */
    float kd = 0.7f, ks = 0.7f, ke = 0.3f;	// kd:�Ȼ�ȿ�͸�, ks:����ȿ�͸�, ke:�Ķ����η���
    /* ���������Ť�׻� */
    rgb_t white;
    set_color(&white, 255, 255, 255);		// �Ķ����ο����������
    rgb_t c, c0, c1;
    float ln = innerproduct_vector(light, n);
    float lv = innerproduct_vector(light, view);
    float nv = innerproduct_vector(n, view);
    float cos_alpha = Max((-1.0f * ln), 0.0f);
    float cos_bata  = Max((2.0f*ln * nv - lv),0.0f);
    float cos_bata_pow20;
    Pow(cos_bata_pow20, cos_bata, 20);			    // ���μ����˱ƶ�. �礭���ʤ�ۤɶ�°Ū�ˤʤ�
    mult_color(&c0, color,  (s * kd * cos_alpha + ke));
    mult_color(&c1, &white, (s * ks * cos_bata_pow20));
    add_color(&c, &c0, &c1);
    return c;
}

/* �쥤�ȥ졼���η׻� */
void trace_ray(image_t* img, scene_data_t* scene, workarea_t* warea) {
    unsigned int t0 = 0, intersect_time = 0, t1 = 0, shading_time = 0;
    int x, y;
    rgb_t color;

    int w = img->width;
    int h = img->height;
    vector_t view = scene->view;
    vector_t viewpoint = scene->viewpoint;
    vector_t light = scene->light;
    sphere_t ball = scene->ball;
    rgb_t bgcolor = scene->bgcolor;

	/* ���쥤�ȥ졼���η׻��ϰ� */
    for (y = warea->starty; y < warea->endy; ++y) {
        viewpoint.y = y*2.0f/h-1.0f;
        for (x = warea->startx; x < warea->endx; ++x) {
            /* �������٥��ȥ����� */
            viewpoint.x = x*2.0f/w-1.0f;	    
            float dv = sqrtf(Sq(viewpoint.x)+Sq(viewpoint.y)+Sq(viewpoint.z));
            set_vector(&view, x, y, -dv);   // &view:vector_t�Υ��ɥ쥹�������, vector_t��x, y, z
            sub_vector(&view, &view, &viewpoint);
            norm_vector(&view);	    // ��᤿�����٥��ȥ�view�ϡ�norm_vector�ؿ��ˤ�ä�ɸ�ಽ
            int index = y*w+x;
            /* ����Ƚ����� �� �����٥��ȥ뤬���Τȸ��뤫�ɤ���*/
#ifdef DEBUG
    start_timer(&t0);
#endif
            float t = intersect(&ball, &viewpoint, &view);  // intersect : ������ϸ����η���t�����ʤ����ϡ���֤�
							    // ����֤�����硢�������طʤǤ���
#ifdef DEBUG
	    intersect_time += stop_timer(&t0);
#endif
            if (t < INFINITY) {				    // �����ȸ����t�ˤ��ä����
                /* ���������ǥ��󥰽����˻��Ѥ���ѥ�᡼���η׻� */
                vector_t p, tv, n, L;
		// �����٥��ȥ�ȵ��ΤȤθ����ˤ�����ˡ���٥��ȥ�����
                mult_vector(&tv, &view, t);	    // ���� t ���Ѥ��Ƹ����٥��ȥ� p �����
                add_vector(&p, &viewpoint, &tv);    // �����٥��ȥ� p �ȵ��Τ��濴�٥��ȥ뤫��ˡ���٥��ȥ� n �����
                sub_vector(&n, &p, &ball.center);
                norm_vector(&n);		    // ˡ���٥��ȥ� n ��ɸ�ಽ
		// ��������ȯ�ͤ��������٥��ȥ�����
                sub_vector(&L, &p, &light);	    // �����٥��ȥ� p �ȸ����٥��ȥ뤫������٥��ȥ� L �����
                norm_vector(&L);
                /* ���������ǥ��󥰽��� */
#ifdef DEBUG
                start_timer(&t1);
#endif
                color = shading(&view, &L, &n, &ball.color, 0.5f);	// �������ǥ��󥰽���
#ifdef DEBUG
                shading_time += stop_timer(&t1);
#endif
                draw_pixel(&img->buf[index], &color);
            } else {
		/* ���طʿ������� */
                draw_pixel(&img->buf[index], &bgcolor);	    // �������طʡʡ�ˤ��ä���硢�����κ�ɸ���Ф����طʿ�������
            }
        }
    }

#ifdef DEBUG
    print_timer(intersect_time);
    print_timer(shading_time);
#endif

    return;
}

int main(int argc, char* argv[]) {
    unsigned int t, time;
    image_t img;
    scene_data_t scene;
    workarea_t warea[THREAD_NUM];

    thread_arg_t targ[THREAD_NUM]; //�ɲ�
    pthread_t tid[THREAD_NUM]; //�ɲ�

    thread_arg_t arg[THREAD_NUM]; //�ɲ�

    int w = IMG_WIDTH;
    int h = IMG_HEIGHT;

    new_image(&img, w, h);		// ��������ݤ��뤿��Υ����ΰ�����
    new_scene(&scene);			// �쥤�ȥ졼���󥰷׻��Τ���Υ�����ν����

    start_timer(&t);

    /* ������åɴ�Ϣ���� */
    init_trace_thread(THREAD_NUM, targ, warea, &img, &scene, w, h);	// 2�ĤΥ���åɤκ���ΰ�ȥѥ�᡼��������
    start_trace_thread(THREAD_NUM, tid, targ);				// ����å�����, �����γ���
    wait_trace_thread(THREAD_NUM, tid);					// ��ȥ���åɽ�λ���Ե�
    trace_ray_thread(arg); // �ɲ�

    time = stop_timer(&t);
    print_timer(time);
    write_ppm(&img, OUTPUT);		// OUTPUT�ǻ��ꤵ��Ƥ���"out.ppm"�Ȥ����ե�����̾�ǽ��Ϥ����
    delete_image(&img);

    return 0;
}

