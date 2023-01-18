/* CS/CNS 171
 * Written by Kevin (Kevli) Li (Class of 2016)
 * Originally for Fall 2014
 *
 * This OpenGL demo code is supposed to introduce you to the OpenGL syntax and
 * to good coding practices when writing programs in OpenGL.
 *
 * The example syntax and code organization in this file should hopefully be
 * good references for you to write your own OpenGL code.
 *
 * The advantage of OpenGL is that it turns a lot of complicated procedures
 * (such as the lighting and shading computations in Assignment 2) into simple
 * calls to built-in library functions. OpenGL also provides an easy way to
 * make mouse and keyboard user interfaces, allowing you to make programs that
 * actually let you interact with the graphics instead of just generating
 * static images. OpenGL is in general a nice tool for when you want to make a
 * quick-and-dirty graphics program.
 *
 * Keep in mind that this demo code uses OpenGL 3.0. 3.0 is not the newest
 * version of OpenGL, but it is stable; and it contains all the necessary
 * functionality for this class. Most of the syntax in 3.0 carries over to
 * the newer versions, so you should still be able to use more modern OpenGL
 * without too much difficulty after this class. The main difference between
 * 3.0 and the newer versions is that 3.0 depends on glut, which has been
 * deprecated on Mac OS.
 *
 * This demo does not cover the OpenGL Shading Language (GLSL for short).
 * GLSL will be covered in a future demo and assignment.
 *
 * Note that if you are looking at this code before having completed
 * Assignments 1 and 2, then you will probably have a hard time understanding
 * a lot of what is going on.
 *
 * The overall idea of what this program does is given on the
 * "System Recommendations and Installation Instructions" page of the class
 * website.
 */
 

/* The following 2 headers contain all the main functions, data structures, and
 * variables that allow for OpenGL development.
 */
#include <GL/glew.h>
#include <GL/glut.h>

/* You will almost always want to include the math library. For those that do
 * not know, the '_USE_MATH_DEFINES' line allows you to use the syntax 'M_PI'
 * to represent pi to double precision in C++. OpenGL works in degrees for
 * angles, so converting between degrees and radians is a common task when
 * working in OpenGL.
 *
 * Besides the use of 'M_PI', the trigometric functions also show up a lot in
 * graphics computations.
 */
#include <math.h>
#define _USE_MATH_DEFINES

/* Standard libraries that are just generally useful. */
#include <iostream>
#include <vector>

/* Libraries used for file parsing */
#include <fstream>
#include <sstream>

/* Map library used to store objects by name */
#include <map>

/* Eigen Library included for ArcBall */
#include <Eigen/Dense>
using Eigen::Vector3f;
using Eigen::Matrix4f;

/* Local libraries for half edge */
#include "structs.h"
#include "halfedge.h"

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////

/* The following are function prototypes for the functions that you will most
 * often write when working in OpenGL.
 *
 * Details on the functions will be given in their respective implementations
 * further below.
 */

void init(string filename);
void reshape(int width, int height);
void display(void);

void init_lights();
void set_lights();
void draw_objects();
void destroy_objects();

void mouse_pressed(int button, int state, int x, int y);
void mouse_moved(int x, int y);
void key_pressed(unsigned char key, int x, int y);

void parseFormatFile(string filename);

///////////////////////////////////////////////////////////////////////////////////////////////////

/* The following structs do not involve OpenGL, but they are useful ways to
 * store information needed for rendering,
 *
 * After Assignment 2, the 3D shaded surface renderer assignment, you should
 * have a fairly intuitive understanding of what these structs represent.
 */


/* The following struct is used for representing a point light.
 *
 * Note that the position is represented in homogeneous coordinates rather than
 * the simple Cartesian coordinates that we would normally use. This is because
 * OpenGL requires us to specify a w-coordinate when we specify the positions
 * of our point lights. We specify the positions in the 'set_lights' function.
 */
struct Point_Light
{
    /* Index 0 has the x-coordinate
     * Index 1 has the y-coordinate
     * Index 2 has the z-coordinate
     * Index 3 has the w-coordinate
     */
    float position[4];
    
    /* Index 0 has the r-component
     * Index 1 has the g-component
     * Index 2 has the b-component
     */
    float color[3];
    
    /* This is our 'k' factor for attenuation as discussed in the lecture notes
     * and extra credit of Assignment 2.
     */
    float attenuation_k;
};
 

/* The following struct is used for storing a set of transformations.
 * Please note that this structure assumes that our scenes will give
 * sets of transformations in the form of transltion -> rotation -> scaling.
 * Obviously this will not be the case for your scenes. Keep this in
 * mind when writing your own programs.
 *
 * Note that we do not need to use matrices this time to represent the
 * transformations. This is because OpenGL will handle all the matrix
 * operations for us when we have it apply the transformations. All we
 * need to do is supply the parameters.
 */

enum transformType { translation, rotation, scaling };

struct Transform {
    /* Indicates type of transformation*/
    transformType type;

    /* For translation, rotation, and scaling:
     * Index 0 has the x-component
     * Index 1 has the y-component
     * Index 2 has the z-component
     * 
     * Index 3 has the angle for rotation only
     */
    float data[4];
};

struct Instance
{
        
    /* Index 0 has the r-component
     * Index 1 has the g-component
     * Index 2 has the b-component
     */
    float ambient_reflect[3];
    float diffuse_reflect[3];
    float specular_reflect[3];
    
    float shininess;

    vector<Transform> transforms;
};

struct Quarternion
{
    float real;
    Vec3f im;
};

Quarternion getIdentityQuarternion(void) {
    Quarternion q;
    q.real = 1;
    q.im = (Vec3f) {0.0f, 0.0f, 0.0f};
    return q;
}


/* The following struct is used to represent objects.
 *
 * The main things to note here are the 'vertex_buffer' and 'normal_buffer'
 * vectors.
 *
 * You will see later in the 'draw_objects' function that OpenGL requires
 * us to supply it all the faces that make up an object in one giant
 * "vertex array" before it can render the object. The faces are each specified
 * by the set of vertices that make up the face, and the giant "vertex array"
 * stores all these sets of vertices consecutively. Our "vertex_buffer" vector
 * below will be our "vertex array" for the object.
 *
 * As an example, let's say that we have a cube object. A cube has 6 faces,
 * each with 4 vertices. Each face is going to be represented by the 4 vertices
 * that make it up. We are going to put each of these 4-vertex-sets one by one
 * into 1 large array. This gives us an array of 36 vertices. e.g.:
 *
 * [face1vertex1, face1vertex2, face1vertex3, face1vertex4,
 *  face2vertex1, face2vertex2, face2vertex3, face2vertex4,
 *  face3vertex1, face3vertex2, face3vertex3, face3vertex4,
 *  face4vertex1, face4vertex2, face4vertex3, face4vertex4,
 *  face5vertex1, face5vertex2, face5vertex3, face5vertex4,
 *  face6vertex1, face6vertex2, face6vertex3, face6vertex4]
 *
 * This array of 36 vertices becomes our 'vertex_array'.
 *
 * While it may be obvious to us that some of the vertices in the array are
 * repeats, OpenGL has no way of knowing this. The redundancy is necessary
 * since OpenGL needs the vertices of every face to be explicitly given.
 *
 * The 'normal_buffer' stores all the normals corresponding to the vertices
 * in the 'vertex_buffer'. With the cube example, since the "vertex array"
 * has "36" vertices, the "normal array" also has "36" normals.
 */
struct Object
{
    vector<Vertex> vertex_buffer;
    vector<Vec3f> normal_buffer;

    Mesh_Data *mesh;
    vector<HEV *> *hevs; // normals stored here
    vector<HEF *> *hefs;
    
    vector<Instance> instances;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

// Time step given by the user that controls the speed of the smoothing 
float time_step_h;
// The char key that starts the smoothing
const char start_smoothing_key = ' ';
// The time in milliseconds between each smoothing frame
static const int FRAME_RATE = 70;

// Tracks if the smoothing has started via the press of the key indicated by start_smoothing_key
bool started_smoothing = false;

///////////////////////////////////////////////////////////////////////////////////////////////////

/* The following are the typical camera specifications and parameters. In
 * general, it is a better idea to keep all this information in a camera
 * struct, like how we have been doing it in Assignemtns 1 and 2. However,
 * if you only have one camera for the scene, and all your code is in one
 * file (like this one), then it is sometimes more convenient to just have
 * all the camera specifications and parameters as global variables.
 */
 
/* Index 0 has the x-coordinate
 * Index 1 has the y-coordinate
 * Index 2 has the z-coordinate
 */
float cam_position[3];
float cam_orientation_axis[3];

/* Angle in degrees.
 */ 
float cam_orientation_angle;

float near_param, far_param,
      left_param, right_param,
      top_param, bottom_param;

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Self-explanatory lists of lights and map of objects.
 */

/* All the lights in the scene */
vector<Point_Light> lights;
/* All the objects mapped by name (filename) */
map<string, Object> objects;

///////////////////////////////////////////////////////////////////////////////////////////////////

/* Quarternions that control ArcBall Rotations
 */
Quarternion last_rotation;
Quarternion curr_rotation;

///////////////////////////////////////////////////////////////////////////////////////////////////

/* The following are parameters for creating an interactive first-person camera
 * view of the scene. The variables will make more sense when explained in
 * context, so you should just look at the 'mousePressed', 'mouseMoved', and
 * 'keyPressed' functions for the details.
 */

int mouse_x, mouse_y;
float mouse_scale_x, mouse_scale_y;

const float step_size = 0.2;
const float x_view_step = 90.0, y_view_step = 90.0;
float x_view_angle = 0, y_view_angle = 0;

bool is_pressed = false;
bool wireframe_mode = false;

///////////////////////////////////////////////////////////////////////////////////////////////////

/* From here on are all the function implementations.
 */
 

/* 'init' function:
 * 
 * As you would expect, the 'init' function initializes and sets up the
 * program. It should always be called before anything else.
 *
 * Writing an 'init' function is not required by OpenGL. If you wanted to, you
 * could just put all your initializations in the beginning of the 'main'
 * function instead. However, doing so is bad style; it is cleaner to have all
 * your initializations contained within one function.
 * 
 * Before we go into the function itself, it is important to mention that
 * OpenGL works like a state machine. It will do different procedures depending
 * on what state it is in.
 *
 * For instance, OpenGL has different states for its shading procedure. By
 * default, OpenGL is in "flat shading state", meaning it will always use flat
 * shading when we tell it to render anything. With some syntax, we can change
 * the shading procedure from the "flat shading state" to the "Gouraud shading
 * state", and then OpenGL will render everything using Gouraud shading.
 *
 * The most important task of the 'init' function is to set OpenGL to the
 * states that we want it to be in.
 */
void init(string filename)
{
    /* Extracts all information from format file entered in command line */
    parseFormatFile(filename);

    /* Rotation Quarternion Initializations */
    last_rotation = getIdentityQuarternion();
    curr_rotation = getIdentityQuarternion();

    /* The following line of code tells OpenGL to use "smooth shading" (aka
     * Gouraud shading) when rendering.
     *
     * Yes. This is actually all you need to do to use Gouraud shading in
     * OpenGL (besides providing OpenGL the vertices and normals to render).
     * Short and sweet, right?
     *
     * If you wanted to tell OpenGL to use flat shading at any point, then you
     * would use the following line:
     
       glShadeModel(GL_FLAT);
     
     * Phong shading unfortunately requires GLSL, so it will be covered in a
     * later demo.
     */
    glShadeModel(GL_SMOOTH);
    
    /* The next line of code tells OpenGL to use "culling" when rendering. The
     * line right after it tells OpenGL that the particular "culling" technique
     * we want it to use is backface culling.
     *
     * "Culling" is actually a generic term for various algorithms that
     * prevent the rendering process from trying to render unnecessary
     * polygons. Backface culling is the most commonly used method, but
     * there also exist other, niche methods like frontface culling.
     */
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    /* The following line tells OpenGL to use depth buffering when rendering.
     */
    glEnable(GL_DEPTH_TEST);
    
     /* The following line tells OpenGL to automatically normalize our normal
     * vectors before it passes them into the normal arrays discussed below.
     * This is required for correct lighting, but it also slows down our
     * program. An alternative to this is to manually scale the normal vectors
     * to correct for each scale operation we call. For instance, if we were
     * to scale an object by 3 (via glScalef() discussed below), then
     * OpenGL would scale the normals of the object by 1/3, as we would
     * expect from the inverse normal transform. But since we need unit
     * normals for lighting, we would either need to enable GL_NORMALIZE
     * or manually scale our normals by 3 before passing them into the
     * normal arrays; this is of course to counteract the 1/3 inverse
     * scaling when OpenGL applies the normal transforms. Enabling GL_NORMALIZE
     * is more convenient, but we sometimes don't use it if it slows down
     * our program too much.
     */
    glEnable(GL_NORMALIZE);
    
    /* The following two lines tell OpenGL to enable its "vertex array" and
     * "normal array" functionality. More details on these arrays are given
     * in the comments on the 'Object' struct and the 'draw_objects' and
     * 'create_objects' functions.
     */
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    
    /* The next 4 lines work with OpenGL's two main matrices: the "Projection
     * Matrix" and the "Modelview Matrix". Only one of these two main matrices
     * can be modified at any given time. We specify the main matrix that we
     * want to modify with the 'glMatrixMode' function.
     *
     * The Projection Matrix is the matrix that OpenGL applies to points in
     * camera space. For our purposes, we want the Projection Matrix to be
     * the perspective projection matrix, since we want to convert points into
     * NDC after they are in camera space.
     *
     * The line of code below:
     */
    glMatrixMode(GL_PROJECTION);
    /* ^tells OpenGL that we are going to modify the Projection Matrix. From
     * this point on, any matrix comamnds we give OpenGL will affect the
     * Projection Matrix. For instance, the line of code below:
     */
    glLoadIdentity();
    /* ^tells OpenGL to set the current main matrix (which is the Projection
     * Matrix right now) to the identity matrix. Then, the next line of code:
     */
    glFrustum(left_param, right_param,
              bottom_param, top_param,
              near_param, far_param);
    /* ^ tells OpenGL to create a perspective projection matrix using the
     * given frustum parameters. OpenGL then post-multiplies the current main
     * matrix (the Projection Matrix) with the created matrix. i.e. let 'P'
     * be our Projection Matrix and 'F' be the matrix created by 'glFrustum'.
     * Then, after 'F' is created, OpenGL performs the following operation:
     *
     * P = P * F
     * 
     * Since we had set the Projection Matrix to the identity matrix before the
     * call to 'glFrustum', the above multiplication results in the Projection
     * Matrix being the perspective projection matrix, which is what we want.
     */
    
    /* The Modelview Matrix is the matrix that OpenGL applies to untransformed
     * points in world space. OpenGL applies the Modelview Matrix to points
     * BEFORE it applies the Projection Matrix.
     * 
     * Thus, for our purposes, we want the Modelview Matrix to be the overall
     * transformation matrix that we apply to points in world space before
     * applying the perspective projection matrix. This means we would need to
     * factor in all the individual object transformations and the camera
     * transformations into the Modelview Matrix.
     *
     * The following line of code tells OpenGL that we are going to modify the
     * Modelview Matrix. From this point on, any matrix commands we give OpenGL
     * will affect the Modelview Matrix.
     *
     * We generally modify the Modelview Matrix in the 'display' function,
     * right before we tell OpenGL to render anything. See the 'display'
     * for details.
     */
    glMatrixMode(GL_MODELVIEW);
    
    /* The next line calls our function that tells OpenGL to initialize some
     * lights to represent our Point Light structs. Further details will be
     * given in the function itself.
     *
     * The reason we have this procedure as a separate function is to make
     * the code more organized.
     */
    init_lights();
}

/* 'reshape' function:
 * 
 * You will see down below in the 'main' function that whenever we create a
 * window in OpenGL, we have to specify a function for OpenGL to call whenever
 * the window resizes. We typically call this function 'reshape' or 'resize'.
 * 
 * The 'reshape' function is supposed to tell your program how to react
 * whenever the program window is resized. It is also called in the beginning
 * when the window is first created. You can think of the first call to
 * 'reshape' as an initialization phase and all subsequent calls as update
 * phases.
 * 
 * Anything that needs to know the dimensions of the program window should
 * be initialized and updated in the 'reshape' function. You will see below
 * that we use the 'reshape' function to initialize and update the conversion
 * scheme between NDC and screen coordinates as well as the mouse interaction
 * parameters.
 */
void reshape(int width, int height)
{
    /* The following two lines of code prevent the width and height of the
     * window from ever becoming 0 to prevent divide by 0 errors later.
     * Typically, we let 1x1 square pixel be the smallest size for the window.
     */
    height = (height == 0) ? 1 : height;
    width = (width == 0) ? 1 : width;
    
    /* The 'glViewport' function tells OpenGL to determine how to convert from
     * NDC to screen coordinates given the dimensions of the window. The
     * parameters for 'glViewport' are (in the following order):
     *
     * - int x: x-coordinate of the lower-left corner of the window in pixels
     * - int y: y-coordinate of the lower-left corner of the window in pixels
     * - int width: width of the window
     * - int height: height of the window
     *
     * We typically just let the lower-left corner be (0,0).
     *
     * After 'glViewport' is called, OpenGL will automatically know how to
     * convert all our points from NDC to screen coordinates when it tries
     * to render them.
     */
    glViewport(0, 0, width, height);
    
    /* The following two lines are specific to updating our mouse interface
     * parameters. Details will be given in the 'mouse_moved' function.
     */
    mouse_scale_x = (float) (right_param - left_param) / (float) width;
    mouse_scale_y = (float) (top_param - bottom_param) / (float) height;
    
    /* The following line tells OpenGL that our program window needs to
     * be re-displayed, meaning everything that was being displayed on
     * the window before it got resized needs to be re-rendered.
     */
    glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////

/* Quarternion Support functions that make ArcBall Rotation Possible */

float screenToNDC(int coord, bool x) {
    if (x) {
        return ( (2.0f * coord) / glutGet(GLUT_INIT_WINDOW_WIDTH) - 0.5f) * (right_param - left_param) + left_param;
    }
    return top_param - ( (2.0f * coord) / glutGet(GLUT_INIT_WINDOW_HEIGHT) - 0.5f) * (top_param - bottom_param);
}

float getZNDC(float x, float y) {
    float squared = x * x + y * y;
    if (squared > 1) {
        return 0.0f;
    }
    return sqrt(1.0f - squared);
}

void computeRotationQuarternion(int x, int y) {
    float x_start = screenToNDC(mouse_x, true);
    float y_start = screenToNDC(mouse_y, false);
    float z_start = getZNDC(x_start, y_start);
    float x_curr = screenToNDC(x, true);
    float y_curr = screenToNDC(y, false);
    float z_curr = getZNDC(x_curr, y_curr);
    Vector3f start (x_start, y_start, z_start);
    Vector3f curr (x_curr, y_curr, z_curr);
    float theta = start.dot(curr) / (start.norm() * curr.norm());
    theta = acos(min(1.0f, theta));
    Vector3f u = start.cross(curr);
    u.normalize();
    double intermediateSinThetaHalf = sin(0.5 * theta);
    curr_rotation.real = -1 * cos(0.5 * theta);
    curr_rotation.im.x = u[0] * intermediateSinThetaHalf;
    curr_rotation.im.y = u[1] * intermediateSinThetaHalf;
    curr_rotation.im.z = u[2] * intermediateSinThetaHalf;
}

Quarternion multiplyQuarternion(Quarternion qa, Quarternion qb) 
{
    Quarternion product;
    Vector3f va (qa.im.x, qa.im.y, qa.im.z);
    Vector3f vb (qb.im.x, qb.im.y, qb.im.z);

    product.real = qa.real * qb.real - va.dot(vb);
    Vector3f v_product = (qa.real * vb) + (qb.real * va) + va.cross(vb);
    product.im.x = v_product[0];
    product.im.y = v_product[1];
    product.im.z = v_product[2];
    return product;
}

void applyArcBallRotation(void) 
{
    Quarternion q = multiplyQuarternion(last_rotation, curr_rotation);
    GLfloat rot[16];

    rot[0] = 1.0f - 2.0f * q.im.y * q.im.y - 2.0f * q.im.z * q.im.z;
    rot[1] = 2.0f * (q.im.x * q.im.y - q.im.z * q.real);
    rot[2] = 2.0f * (q.im.x * q.im.z + q.im.y * q.real);
    rot[3] = 0.0f;

    rot[4] = 2.0f * (q.im.x * q.im.y + q.im.z * q.real);
    rot[5] = 1.0f - 2.0f * q.im.x * q.im.x - 2.0f * q.im.z * q.im.z;
    rot[6] = 2.0f * (q.im.y * q.im.z - q.im.x * q.real);
    rot[7] = 0.0f;

    rot[8] = 2.0f * (q.im.x * q.im.z - q.im.y * q.real);
    rot[9] = 2.0f * (q.im.y * q.im.z + q.im.x * q.real);
    rot[10] = 1.0f - 2.0f * q.im.x * q.im.x - 2.0f * q.im.y * q.im.y;
    rot[11] = 0.0f;
    
    rot[12] = 0.0f;
    rot[13] = 0.0f;
    rot[14] = 0.0f;
    rot[15] = 1.0f;

    glMultMatrixf(rot);
}

//////////////////////////////////////////////////////////////////////////


/* 'display' function:
 * 
 * You will see down below in the 'main' function that whenever we create a
 * window in OpenGL, we have to specify a function for OpenGL to call whenever
 * it wants to render anything. We typically name this function 'display' or
 * 'render'.
 *
 * The 'display' function is supposed to handle all the processing of points
 * in world and camera space.
 */
void display(void)
{
    /* The following line of code is typically the first line of code in any
     * 'display' function. It tells OpenGL to reset the "color buffer" (which
     * is our pixel grid of RGB values) and the depth buffer.
     *
     * Resetting the "color buffer" is equivalent to clearing the program
     * window so that it only displays a black background. This allows OpenGL
     * to render a new scene onto the window without having to deal with the
     * remnants of the previous scene.
     *
     * Resetting the depth buffer sets all the values in the depth buffer back
     * to a very high number. This allows the depth buffer to be reused for
     * rendering a new scene.
     */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    /* With the program window cleared, OpenGL is ready to render a new scene.
     * Of course, before we can render anything correctly, we need to make all
     * the appropriate camera and object transformations to our coordinate
     * space.
     *
     * Recall that the 'init' function used the glMatrixMode function to put
     * OpenGL into a state where we can modify its Modelview Matrix. Also
     * recall that we want the Modelview Matrix to be the overall transform-
     * ation matrix that we apply to points in world space before applying the
     * perspective projection matrix. This means that we need to factor in all
     * the individual object transformations and the camera transformations
     * into the Modelview Matrix.
     *
     * To do so, our first step is to "reset" the Modelview Matrix by setting it
     * to the identity matrix:
     */
    glLoadIdentity();

    /* Now, if you recall, for a given object, we want to FIRST multiply the
     * coordinates of its points by the translations, rotations, and scalings
     * applied to the object and THEN multiply by the inverse camera rotations
     * and translations.
     *
     * HOWEVER, OpenGL modifies the Modelview Matrix using POST-MULTIPLICATION.
     * This means that if were to specify to OpenGL a matrix modification 'A',
     * then letting the Modelview Matrix be 'M', OpenGL would perform the
     * following operation:
     *
     * M = M * A
     *
     * So, for instance, if the Modelview Matrix were initialized to the
     * identity matrix 'I' and we were to specify a translation 'T' followed by
     * a rotation 'R' followed by a scaling 'S' followed by the inverse camera
     * transform 'C', then the Modelview Matrix is modified in the following
     * order:
     * 
     * M = I * T * R * S * C
     * 
     * Then, when OpenGL applies the Modelview Matrix to a point 'p', we would
     * get the following multiplication:
     *
     * M * p = I * T * R * S * C * p
     *
     * ^ So the camera transformation ends up being applied first even though
     * it was specified last. This is not what we want. What we want is
     * something like this:
     *
     * M * p = C * T * R * S * I * p
     *
     * Hence, to correctly transform a point, we actually need to FIRST specify
     * the inverse camera rotations and translations and THEN specify the
     * translations, rotations, and scalings applied to an object.
     *
     * We start by specifying any camera rotations caused by the mouse. We do
     * so by using the 'glRotatef' function, which takes the following parameters
     * in the following order:
     * 
     * - float angle: rotation angle in DEGREES
     * - float x: x-component of rotation axis
     * - float y: y-component of rotation axis
     * - float z: z-component of rotation axis
     *
     * The 'glRotatef' function tells OpenGL to create a rotation matrix using
     * the given angle and rotation axis.
     */
    glRotatef(y_view_angle, 1, 0, 0);
    glRotatef(x_view_angle, 0, 1, 0);
    /* 'y_view_angle' and 'x_view_angle' are parameters for our mouse user
     * interface. They keep track of how much the user wants to rotate the
     * camera from its default, specified orientation. See the 'mouse_moved'
     * function for more details.
     *
     * Our next step is to specify the inverse rotation of the camera by its
     * orientation angle about its orientation axis:
     */
    glRotatef(-cam_orientation_angle,
              cam_orientation_axis[0], cam_orientation_axis[1], cam_orientation_axis[2]);
    /* We then specify the inverse translation of the camera by its position using
     * the 'glTranslatef' function, which takes the following parameters in the
     * following order:
     *
     * - float x: x-component of translation vector
     * - float y: x-component of translation vector
     * - float z: x-component of translation vector
     */
    glTranslatef(-cam_position[0], -cam_position[1], -cam_position[2]);
    /* ^ And that should be it for the camera transformations.
     */
    
    /* Uses the defined Quarternion support functions above to back multiply 
     * the ModelView Matrix by the Quarternion Rotation Matrix so that the scene can 
     * be contained within the rotated ArcBall sphere. 
     * 
     * ModelView = ModelView * Quarternion_Rotation
     */
    applyArcBallRotation();

    /* Our next step is to set up all the lights in their specified positions.
     * Our helper function, 'set_lights' does this for us. See the function
     * for more details.
     *
     * The reason we have this procedure as a separate function is to make
     * the code more organized.
     */
    set_lights();
    /* Once the lights are set, we can specify the points and faces that we
     * want drawn. We do all this in our 'draw_objects' helper function. See
     * the function for more details.
     *
     * The reason we have this procedure as a separate function is to make
     * the code more organized.
     */
    draw_objects();
    
    /* The following line of code has OpenGL do what is known as "double
     * buffering".
     *
     * Imagine this: You have a relatively slow computer that is telling OpenGL
     * to render something to display in the program window. Because your
     * computer is slow, OpenGL ends up rendering only part of the scene before
     * it displays it in the program window. The rest of the scene shows up a
     * second later. This effect is referred to as "flickering". You have most
     * likely experienced this sometime in your life when using a computer. It
     * is not the most visually appealing experience, right?
     *
     * To avoid the above situation, we need to tell OpenGL to display the
     * entire scene at once rather than rendering the scene one pixel at a
     * time. We do so by enabling "double buffering".
     * 
     * Basically, double buffering is a technique where rendering is done using
     * two pixel grids of RGB values. One pixel grid is designated as the
     * "active buffer" while the other is designated as the "off-screen buffer".
     * Rendering is done on the off-screen buffer while the active buffer is
     * being displayed. Once the scene is fully rendered on the off-screen buffer,
     * the two buffers switch places so that the off-screen buffer becomes the
     * new active buffer and gets displayed while the old active buffer becomes
     * the new off-screen buffer. This process allows scenes to be fully rendered
     * onto the screen at once, avoiding the flickering effect.
     * 
     * We actually enable double buffering in the 'main' function with the line:
     
       glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
       
     * ^ 'GLUT_DOUBLE' tells OpenGL to use double buffering. The other two
     * parameters, 'GLUT_RGB' and 'GLUT_DEPTH', tell OpenGL to initialize the
     * RGB pixel grids and the depth buffer respectively.
     *
     * The following function, 'glutSwapBuffers', tells OpenGL to swap the
     * active and off-screen buffers.
     */
    glutSwapBuffers();
}

/* 'init_lights' function:
 * 
 * This function has OpenGL enable its built-in lights to represent our point
 * lights.
 *
 * OpenGL has 8 built-in lights in all, each one with its own unique, integer
 * ID value. When setting the properties of a light, we need to tell OpenGL
 * the ID value of the light we are modifying.
 * 
 * The first light's ID value is stored in 'GL_LIGHT0'. The second light's ID
 * value is stored in 'GL_LIGHT1'. And so on. The eighth and last light's ID
 * value is stored in 'GL_LIGHT7'.
 *
 * The properties of the lights are set using the 'glLightfv' and 'glLightf'
 * functions as you will see below.
 */
void init_lights()
{
    /* The following line of code tells OpenGL to enable lighting calculations
     * during its rendering process. This tells it to automatically apply the
     * Phong reflection model or lighting model to every pixel it will render.
     */
    glEnable(GL_LIGHTING);
    
    int num_lights = lights.size();
    
    for(int i = 0; i < num_lights; ++i)
    {
        /* In this loop, we are going to associate each of our point lights
         * with one of OpenGL's built-in lights. The simplest way to do this
         * is to just let our first point light correspond to 'GL_LIGHT0', our
         * second point light correspond to 'GL_LIGHT1', and so on. i.e. let:
         * 
         * 'lights[0]' have an ID value of 'GL_LIGHT0'
         * 'lights[1]' have an ID value of 'GL_LIGHT1'
         * etc...
         */
        int light_id = GL_LIGHT0 + i;
        
        glEnable(light_id);
        
        /* The following lines of code use 'glLightfv' to set the color of
         * the light. The parameters for 'glLightfv' are:
         *
         * - enum light_ID: an integer between 'GL_LIGHT0' and 'GL_LIGHT7'
         * - enum property: this varies depending on what you are setting
         *                  e.g. 'GL_AMBIENT' for the light's ambient component
         * - float* values: a set of values to set for the specified property
         *                  e.g. an array of RGB values for the light's color
         * 
         * OpenGL actually lets us specify different colors for the ambient,
         * diffuse, and specular components of the light. However, since we
         * are used to only working with one overall light color, we will
         * just set every component to the light color.
         */
        glLightfv(light_id, GL_AMBIENT, lights[i].color);
        glLightfv(light_id, GL_DIFFUSE, lights[i].color);
        glLightfv(light_id, GL_SPECULAR, lights[i].color);
        
        /* The following line of code sets the attenuation k constant of the
         * light. The difference between 'glLightf' and 'glLightfv' is that
         * 'glLightf' is used for when the parameter is only one value like
         * the attenuation constant while 'glLightfv' is used for when the
         * parameter is a set of values like a color array. i.e. the third
         * parameter of 'glLightf' is just a float instead of a float*.
         */
        glLightf(light_id, GL_QUADRATIC_ATTENUATION, lights[i].attenuation_k);
    }
}

/* 'set_lights' function:
 *
 * While the 'init_lights' function enables and sets the colors of the lights,
 * the 'set_lights' function is supposed to position the lights.
 *
 * You might be wondering why we do not just set the positions of the lights in
 * the 'init_lights' function in addition to the other properties. The reason
 * for this is because OpenGL does lighting computations after it applies the
 * Modelview Matrix to points. This means that the lighting computations are
 * effectively done in camera space. Hence, to ensure that we get the correct
 * lighting computations, we need to make sure that we position the lights
 * correctly in camera space.
 * 
 * Now, the 'glLightfv' function, when used to position a light, applies all
 * the current Modelview Matrix to the given light position. This means that
 * to correctly position lights in camera space, we should call the 'glLightfv'
 * function to position them AFTER the Modelview Matrix has been modified by
 * the necessary camera transformations. As you can see in the 'display'
 * function, this is exactly what we do.
 */
void set_lights()
{
    int num_lights = lights.size();
    
    for(int i = 0; i < num_lights; ++i)
    {
        int light_id = GL_LIGHT0 + i;
        
        glLightfv(light_id, GL_POSITION, lights[i].position);
    }
}

/* 'draw_objects' function:
 *
 * This function has OpenGL render our objects to the display screen.
 */
void draw_objects()
{   
    for (map<string, Object>::iterator obj_iter = objects.begin(); 
                                    obj_iter != objects.end(); obj_iter++) {
        Object &obj = objects[obj_iter->first];
        /* The current Modelview Matrix is actually stored at the top of a
         * stack in OpenGL. The following function, 'glPushMatrix', pushes
         * another copy of the current Modelview Matrix onto the top of the
         * stack. This results in the top two matrices on the stack both being
         * the current Modelview Matrix. Let us call the copy on top 'M1' and
         * the copy that is below it 'M2'.
         *
         * The reason we want to use 'glPushMatrix' is because we need to
         * modify the Modelview Matrix differently for each object we need to
         * render, since each object is affected by different transformations.
         * We use 'glPushMatrix' to essentially keep a copy of the Modelview
         * Matrix before it is modified by an object's transformations. This
         * copy is our 'M2'. We then modify 'M1' and use it to render the
         * object. After we finish rendering the object, we will pop 'M1' off
         * the stack with the 'glPopMatrix' function so that 'M2' returns to
         * the top of the stack. This way, we have the old unmodified Modelview
         * Matrix back to edit for the next object we want to render.
         */
        glPushMatrix();
        /* The following brace is not necessary, but it keeps things organized.
         */
        {
            int num_instances = obj.instances.size();
            
            /* The loop tells OpenGL to modify our modelview matrix with the
             * desired geometric transformations for this object. Remember
             * though that our 'transform_sets' struct assumes that transformations
             * are conveniently given in sets of translation -> rotate -> scaling;
             * and THIS IS NOT THE CASE FOR YOUR SCENE FILES. DO NOT BLINDLY
             * COPY THE FOLLOWING CODE.
             *
             * To explain how to correctly transform your objects, consider the
             * following example. Suppose our object has the following desired
             * transformations:
             *
             *    scale by 2, 2, 2
             *    translate by 2, 0, 5
             *    rotate about 0, 1, 0.5 and by angle = 0.6 radians
             *
             * Obviously, we cannot use the following loop for this, because the order
             * is not translate -> rotate -> scale. Instead, we need to make the following
             * calls in this exact order:
             *
             *    glRotatef(0.6 * 180.0 / M_PI, 0, 1, 0.5);
             *    glTranslatef(2, 0, 5);
             *    glScalef(2, 2, 2);
             *
             * We make the calls in the REVERSE order of how the transformations are specified
             * because OpenGL edits our modelview matrix using post-multiplication (see above
             * at the notes regarding the camera transforms in display()).
             *
             * Keep all this in mind to come up with an appropriate way to store and apply
             * geometric transformations for each object in your scenes.
             */
            for (int instanceIdx = 0; instanceIdx < num_instances; ++instanceIdx)
            {
                Instance &inst = obj.instances[instanceIdx];
                int num_transforms = inst.transforms.size();

                /* Applies the transformations in REVERSE order because OpenGL uses
                 * post matrix multiplication. */
                for (int transformIdx = num_transforms - 1; transformIdx >= 0; --transformIdx)
                {
                    switch(inst.transforms[transformIdx].type) {
                        case translation :
                            glTranslatef(inst.transforms[transformIdx].data[0],
                                    inst.transforms[transformIdx].data[1],
                                    inst.transforms[transformIdx].data[2]);
                            break;
                        case rotation :
                            glRotatef(inst.transforms[transformIdx].data[3],
                                    inst.transforms[transformIdx].data[0],
                                    inst.transforms[transformIdx].data[1],
                                    inst.transforms[transformIdx].data[2]);
                            break;
                        case scaling :
                            glScalef(inst.transforms[transformIdx].data[0],
                                    inst.transforms[transformIdx].data[1],
                                    inst.transforms[transformIdx].data[2]);
                    }
                }
            
                /* The 'glMaterialfv' and 'glMaterialf' functions tell OpenGL
                * the material properties of the surface we want to render.
                * The parameters for 'glMaterialfv' are (in the following order):
                *
                * - enum face: Options are 'GL_FRONT' for front-face rendering,
                *              'GL_BACK' for back-face rendering, and
                *              'GL_FRONT_AND_BACK' for rendering both sides.
                * - enum property: this varies on what you are setting up
                *                  e.g. 'GL_AMBIENT' for ambient reflectance
                * - float* values: a set of values for the specified property
                *                  e.g. an array of RGB values for the reflectance
                *
                * The 'glMaterialf' function is the same, except the third
                * parameter is only a single float value instead of an array of
                * values. 'glMaterialf' is used to set the shininess property.
                */
                glMaterialfv(GL_FRONT, GL_AMBIENT, inst.ambient_reflect);
                glMaterialfv(GL_FRONT, GL_DIFFUSE, inst.diffuse_reflect);
                glMaterialfv(GL_FRONT, GL_SPECULAR, inst.specular_reflect);
                glMaterialf(GL_FRONT, GL_SHININESS, inst.shininess);
            
                /* The next few lines of code are how we tell OpenGL to render
                * geometry for us. First, let us look at the 'glVertexPointer'
                * function.
                * 
                * 'glVertexPointer' tells OpenGL the specifications for our
                * "vertex array". As a recap of the comments from the 'Object'
                * struct, the "vertex array" stores all the faces of the surface
                * we want to render. The faces are stored in the array as
                * consecutive points. For instance, if our surface were a cube,
                * then our "vertex array" could be the following:
                *
                * [face1vertex1, face1vertex2, face1vertex3, face1vertex4,
                *  face2vertex1, face2vertex2, face2vertex3, face2vertex4,
                *  face3vertex1, face3vertex2, face3vertex3, face3vertex4,
                *  face4vertex1, face4vertex2, face4vertex3, face4vertex4,
                *  face5vertex1, face5vertex2, face5vertex3, face5vertex4,
                *  face6vertex1, face6vertex2, face6vertex3, face6vertex4]
                * 
                * Obviously to us, some of the vertices in the array are repeats.
                * However, the repeats cannot be avoided since OpenGL requires
                * this explicit specification of the faces.
                *
                * The parameters to the 'glVertexPointer' function are as
                * follows:
                *
                * - int num_points_per_face: this is the parameter that tells
                *                            OpenGL where the breaks between
                *                            faces are in the vertex array.
                *                            Below, we set this parameter to 3,
                *                            which tells OpenGL to treat every
                *                            set of 3 consecutive vertices in
                *                            the vertex array as 1 face. So
                *                            here, our vertex array is an array
                *                            of triangle faces.
                *                            If we were using the example vertex
                *                            array above, we would have set this
                *                            parameter to 4 instead of 3.
                * - enum type_of_coordinates: this parameter tells OpenGL whether
                *                             our vertex coordinates are ints,
                *                             floats, doubles, etc. In our case,
                *                             we are using floats, hence 'GL_FLOAT'.
                * - sizei stride: this parameter specifies the number of bytes
                *                 between consecutive vertices in the array.
                *                 Most often, you will set this parameter to 0
                *                 (i.e. no offset between consecutive vertices).
                * - void* pointer_to_array: this parameter is the pointer to
                *                           our vertex array.
                */
                glVertexPointer(3, GL_FLOAT, 0, &obj.vertex_buffer[0]);
                /* The "normal array" is the equivalent array for normals.
                * Each normal in the normal array corresponds to the vertex
                * of the same index in the vertex array.
                *
                * The 'glNormalPointer' function has the following parameters:
                *
                * - enum type_of_normals: e.g. int, float, double, etc
                * - sizei stride: same as the stride parameter in 'glVertexPointer'
                * - void* pointer_to_array: the pointer to the normal array
                */
                glNormalPointer(GL_FLOAT, 0, &obj.normal_buffer[0]);
                
                int buffer_size = obj.vertex_buffer.size();
                
                if(!wireframe_mode)
                    /* Finally, we tell OpenGL to render everything with the
                    * 'glDrawArrays' function. The parameters are:
                    * 
                    * - enum mode: in our case, we want to render triangles,
                    *              so we specify 'GL_TRIANGLES'. If we wanted
                    *              to render squares, then we would use
                    *              'GL_QUADS' (for quadrilaterals).
                    * - int start_index: the index of the first vertex
                    *                    we want to render in our array
                    * - int num_vertices: number of vertices to render
                    *
                    * As OpenGL renders all the faces, it automatically takes
                    * into account all the specifications we have given it to
                    * do all the lighting calculations for us. It also applies
                    * the Modelview and Projection matrix transformations to
                    * the vertices and converts everything to screen coordinates
                    * using our Viewport specification. Everything is rendered
                    * onto the off-screen buffer.
                    */
                    glDrawArrays(GL_TRIANGLES, 0, buffer_size);
                else
                    /* If we are in "wireframe mode" (see the 'key_pressed'
                    * function for more information), then we want to render
                    * lines instead of triangle surfaces. To render lines,
                    * we use the 'GL_LINE_LOOP' enum for the mode parameter.
                    * However, we need to draw each face frame one at a time
                    * to render the wireframe correctly. We can do so with a
                    * for loop:
                    */
                    for(int j = 0; j < buffer_size; j += 3)
                        glDrawArrays(GL_LINE_LOOP, j, 3);
            }
        }
        /* As discussed before, we use 'glPopMatrix' to get back the
         * version of the Modelview Matrix that we had before we specified
         * the object transformations above. We then move on in our loop
         * to the next object we want to render.
         */
        glPopMatrix();
    }
    
    
    /* The following code segment uses OpenGL's built-in sphere rendering
     * function to render the blue-ground that you are walking on when
     * you run the program. The blue-ground is just the surface of a big
     * sphere of radius 100.
     */
    glPushMatrix();
    {
        glTranslatef(0, -103, 0);
        glutSolidSphere(100, 100, 100);
    }
    glPopMatrix();
}

/* 'mouse_pressed' function:
 * 
 * This function is meant to respond to mouse clicks and releases. The
 * parameters are:
 * 
 * - int button: the button on the mouse that got clicked or released,
 *               represented by an enum
 * - int state: either 'GLUT_DOWN' or 'GLUT_UP' for specifying whether the
 *              button was pressed down or released up respectively
 * - int x: the x screen coordinate of where the mouse was clicked or released
 * - int y: the y screen coordinate of where the mouse was clicked or released
 *
 * The function doesn't really do too much besides set some variables that
 * we need for the 'mouse_moved' function.
 */
void mouse_pressed(int button, int state, int x, int y)
{
    // MOUSE CLICKED
    /* If the left-mouse button was clicked down, then...
     */
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        /* Store the mouse position in our global variables.
         */
        mouse_x = x;
        mouse_y = y;
        
        /* Since the mouse is being pressed down, we set our 'is_pressed"
         * boolean indicator to true.
         */
        is_pressed = true;
    }

    // MOUSE RELEASED
    /* If the left-mouse button was released up, then...
     */
    else if(button == GLUT_LEFT_BUTTON && state == GLUT_UP)
    {
        last_rotation = multiplyQuarternion(last_rotation, curr_rotation);
        curr_rotation = getIdentityQuarternion();

        /* Mouse is no longer being pressed, so set our indicator to false.
         */
        is_pressed = false;
    }
}

/* 'mouse_moved' function:
 *
 * This function is meant to respond to when the mouse is being moved. There
 * are just two parameters to this function:
 * 
 * - int x: the x screen coordinate of where the mouse was clicked or released
 * - int y: the y screen coordinate of where the mouse was clicked or released
 *
 * We compute our camera rotation angles based on the mouse movement in this
 * function.
 */
void mouse_moved(int x, int y)
{
    /* If the left-mouse button is being clicked down...
     */
    if(is_pressed)
    {
        /* Saves the rotation that needs to be done for where the mouse 
         * is now as the current rotation. */
        computeRotationQuarternion(x, y);
        
        /* Tell OpenGL that it needs to re-render our scene with the new camera
         * angles.
         */
        glutPostRedisplay();
    }
}

/* 'deg2rad' function:
 * 
 * Converts given angle in degrees to radians.
 */
float deg2rad(float angle)
{
    return angle * M_PI / 180.0;
}

/* 'rad2deg' function:
 * 
 * Converts given angle in radians to degrees.
 */
float rad2deg(float angle)
{
    return angle * 180.0 / M_PI;
}


// Computes the area-weighted normal of a vertex using the halfedge data
Vec3f *calculateVertexNormal(HEV *vertex)
{
    Vector3f normal (0.0f, 0.0f, 0.0f);

    // Get the halfedge outgoing from our given vertex
    HE* he = vertex->out; 

    // Loops to all adjacent vertices of our given vertex
    do {
        // Gets the 3 vertices of the triangle face
        HEV *v1 = he->face->edge->vertex;
        HEV *v2 = he->face->edge->next->vertex;
        HEV *v3 = he->face->edge->next->next->vertex;

        // Converts the vertices to Eigen Vectors for computations
        Vector3f v1Vec (v1->x, v1->y, v1->z);
        Vector3f v2Vec (v2->x, v2->y, v2->z);
        Vector3f v3Vec (v3->x, v3->y, v3->z);

        // Computes the normal of the plane of the face
        Vector3f face_normal = (v2Vec - v1Vec).cross(v3Vec - v1Vec);
        
        // Computes the area of the triangular face
        float face_area = 0.5 * face_normal.norm();

        // Accumulates the area-weighted component into our normal
        normal = normal + face_area * face_normal;

        // Gets the halfedge to the next adjacent vertex
        he = he->flip->next;

    } while(he != vertex->out);

    // Normalizes our computed normal
    normal.normalize();

    // Converts our normal into a Vec3f and returns it
    Vec3f *n = new Vec3f;
    *n = {normal[0], normal[1], normal[2]};
    return n;
}


/* Computes all vertex normals and stores them in the 
 * given object's normal buffer using HE data structures.
 * Note: Assumes the HEV data structures have already been built for the object.
 */
void computeUpdateNormals(Object &obj) {
    // Computes and stores all the area-weighted vertex normals
    for (int vIdx = 1; vIdx < obj.hevs->size(); vIdx++) {
        HEV *hev = obj.hevs->at(vIdx);
        Vec3f *normal = calculateVertexNormal(hev);
        hev->normal = *normal;
    }

    // Populates normal buffer using our mesh data and computed normals
    for (int fIdx = 0; fIdx < obj.mesh->faces->size(); fIdx++) {
        Face *f = obj.mesh->faces->at(fIdx);

        // First Normal of the Face 
        Vec3f n1 = obj.hevs->at(f->idx1)->normal;
        obj.normal_buffer.push_back(n1);

        // Second Normal of the Face 
        Vec3f n2 = obj.hevs->at(f->idx2)->normal;
        obj.normal_buffer.push_back(n2);

        // Third Normal of the Face 
        Vec3f n3 = obj.hevs->at(f->idx3)->normal;
        obj.normal_buffer.push_back(n3);
    }
}


void splitBySpace(string s, vector<string> &split)
{
    stringstream stream(s);

    string buffer;
    while(getline(stream, buffer, ' ')) {
        split.push_back(buffer);
    }
}


void parseObjFile(string filename, Object &obj)
{
    // Opens the obj file and prepares to parse
    if (filename.find(".obj") == -1) {
        throw invalid_argument("File " + filename + " needs to be a .obj file.");
    }
    string buffer;
    ifstream file;
    file.open(filename.c_str(), ifstream::in);
    if (file.fail()) {
        string msg = "Could not read obj file '" + filename + "'.";
        throw invalid_argument(msg);
    }

    // Initializes the mesh and 1-indexes its vertices
    obj.mesh = new Mesh_Data;
    obj.mesh->vertices = new vector<Vertex *>();
    obj.mesh->faces = new vector<Face *>();
    obj.mesh->vertices->push_back(NULL);

    // Reads in the vertices and faces into the object's mesh
    int count = 1;
    vector<string> element;
    while (getline(file, buffer)) {
        element.clear();
        splitBySpace(buffer, element);

        // Parses the line as a vertex
        if (element[0] == "v") {
            Vertex *vert = new Vertex;
            *vert = {stof(element[1]), stof(element[2]), stof(element[3])};
            obj.mesh->vertices->push_back(vert);

            continue;
        } 

        /* If not a vertex, parses the line as a face */
        Face *f = new Face;
        *f = {stoi(element[1]), stoi(element[2]), stoi(element[3])};
        obj.mesh->faces->push_back(f);
    }

    // Populates vertex buffer using our mesh data
    for (int fIdx = 0; fIdx < obj.mesh->faces->size(); fIdx++) {
        Face *f = obj.mesh->faces->at(fIdx);

        // First Vertex of the Face 
        Vertex *v1 = obj.mesh->vertices->at(f->idx1);
        obj.vertex_buffer.push_back(*v1);

        // Second Vertex of the Face 
        Vertex *v2 = obj.mesh->vertices->at(f->idx2);
        obj.vertex_buffer.push_back(*v2);

        // Third Vertex of the Face 
        Vertex *v3 = obj.mesh->vertices->at(f->idx3);
        obj.vertex_buffer.push_back(*v3);
    }

    // Builds the halfedge structures
    obj.hevs = new vector<HEV *>();
    obj.hefs = new vector<HEF *>();
    build_HE(obj.mesh, obj.hevs, obj.hefs);

    // Computes all vertex normals and stores them in the object's normal buffer using HE structures
    computeUpdateNormals(obj);

    // Closes the file once done
    file.close();
}

/** 
 * Populates all global fields not already filled with information extracted 
 * by parsing the format file that was entered in the command line.
 * 
 * @param filename, the filename entered in the command line
 * @throws invalid_argument if it fails to read the file
 */ 
void parseFormatFile(string filename)
{
    if (filename.find(".txt") == -1) {
        throw invalid_argument("File " + filename + " needs to be a .txt file.");
    }

    string buffer;
    ifstream file;
    file.open(filename.c_str(), ifstream::in);
    if (file.fail()) {
        throw invalid_argument("Could not read format file '" + filename + "'.");
    }

    /* Saves the directory where scene file and its obj files are stored */
    string directory = filename;
    directory.erase(directory.find_last_of('/') + 1);

    vector<string> line;

    /* Reads in camera and perspective parameters */
    while (getline(file, buffer)) {
        line.clear();
        splitBySpace(buffer, line);

        if (line.size() == 0) {
            break;
        } else if (line[0] == "position") {
            cam_position[0] = stof(line[1]);
            cam_position[1] = stof(line[2]);
            cam_position[2] = stof(line[3]);
        } else if (line[0] == "orientation") {
            cam_orientation_axis[0] = stof(line[1]);
            cam_orientation_axis[1] = stof(line[2]);
            cam_orientation_axis[2] = stof(line[3]);
            cam_orientation_angle = rad2deg(stof(line[4]));
        } else if (line[0] == "near") {
            near_param = stof(line[1]);
        } else if (line[0] == "far") {
            far_param = stof(line[1]);
        } else if (line[0] == "left") {
            left_param = stof(line[1]);
        } else if (line[0] == "right") {
            right_param = stof(line[1]);
        } else if (line[0] == "top") {
            top_param = stof(line[1]);
        } else if (line[0] == "bottom") {
            bottom_param = stof(line[1]);
        }
    }

    /* Reads in all point light sources */
    while (getline(file, buffer)) {
        line.clear();
        splitBySpace(buffer, line);

        if (line.size() == 0) {
            break;
        }

        Point_Light light;

        light.position[0] = stof(line[1]);
        light.position[1] = stof(line[2]);
        light.position[2] = stof(line[3]);
        light.position[3] = 1;

        light.color[0] = stof(line[5]);
        light.color[1] = stof(line[6]);
        light.color[2] = stof(line[7]);

        light.attenuation_k = stof(line[9]);
        
        lights.push_back(light);
    }

    /* Reads in all objects storing them in the objects map */
    while (getline(file, buffer)) {
        line.clear();
        splitBySpace(buffer, line);

        if (line.size() == 0) {
            break;
        }

        if (line[0] == "objects:") {
            continue;
        }

        /* Reinitializes obj for each new object we need */
        Object obj;
        parseObjFile(directory + line[1], obj);
        objects.insert(pair<string, Object>(line[0], obj));
    }

    /* Reads in all objects instances */
    Object *currObj = NULL;
    int instanceIdx;
    while (getline(file, buffer)) {
        line.clear();
        splitBySpace(buffer, line);

        /* If the current object is empty, we are ready to read in a instance.

         * In our scene, each object (in objects map) only acts as a template
         * and uses instances to describe specific modifications of itself
         * that actually get rendered to the screen.
         * 
         * In the format file, the start of an instance is indicated via:
         *      [object name] [object filename]
         * We use [object name] to grab the object from the object map.
         * We make a instance and set a new instanceIndex to prepare 
         * for processing.
         */
        if (currObj == NULL) {
            currObj = &objects[line[0]];
            instanceIdx = currObj->instances.size();
            Instance inst;
            currObj->instances.push_back(inst);
            continue;
        }

        /* Reset the current object to empty if we've reached the end of a 
         * instance description. */
        if (line.size() == 0) {
            currObj = NULL;
            continue;
        }

        /* Processes the reflectance parameters for the transform set */
        if (line[0] == "ambient") {
            currObj->instances[instanceIdx].ambient_reflect[0] = stof(line[1]);
            currObj->instances[instanceIdx].ambient_reflect[1] = stof(line[2]);
            currObj->instances[instanceIdx].ambient_reflect[2] = stof(line[3]);
            continue;
        } else if (line[0] == "diffuse") {
            currObj->instances[instanceIdx].diffuse_reflect[0] = stof(line[1]);
            currObj->instances[instanceIdx].diffuse_reflect[1] = stof(line[2]);
            currObj->instances[instanceIdx].diffuse_reflect[2] = stof(line[3]);
            continue;
        } else if (line[0] == "specular") {
            currObj->instances[instanceIdx].specular_reflect[0] = stof(line[1]);
            currObj->instances[instanceIdx].specular_reflect[1] = stof(line[2]);
            currObj->instances[instanceIdx].specular_reflect[2] = stof(line[3]);
            continue;
        } else if (line[0] == "shininess") {
            currObj->instances[instanceIdx].shininess = stof(line[1]);
            continue;
        }

        /* Processes one line of the file as a transform */
        Transform transformation;
        transformation.data[0] = stof(line[1]);
        transformation.data[1] = stof(line[2]);
        transformation.data[2] = stof(line[3]);
        if (line[0][0] == 't') {
            transformation.type = translation;
        } else if (line[0][0] == 'r') {
            transformation.type = rotation;
            transformation.data[3] = rad2deg(stof(line[4])); 
        } else {
            transformation.type = scaling;
        }

        /* Adds the transform to the object instance's list of tranforms */
        currObj->instances[instanceIdx].transforms.push_back(transformation);
    }
   
    file.close();
}


void computeSmoothing(Object &obj) {
    ;
}


// Smoothes and displays the next frame at a set regular rate
void smoothNextFrame(int rate) {
    // Computes the Smoothing and New Normals for every Object
    for (map<string, Object>::iterator obj_iter = objects.begin(); 
                                    obj_iter != objects.end(); obj_iter++) {
        Object &obj = objects[obj_iter->first];
        computeSmoothing(obj);
        computeUpdateNormals(obj);
    }

    // Redisplays the scene with new smoothed objects
    glutPostRedisplay();

    // Sets the next smoothing to occur at the given regular rate
    glutTimerFunc(rate, smoothNextFrame, rate);
}


/* 'key_pressed' function:
 * 
 * This function is meant to respond to key pressed on the keyboard. The
 * parameters are:
 *
 * - unsigned char key: the character of the key itself or the ASCII value of
 *                      of the key
 * - int x: the x screen coordinate of where the mouse was when the key was pressed
 * - int y: the y screen coordinate of where the mouse was when the key was pressed
 *
 * Our function is pretty straightforward as you can see below. We also do not make
 * use of the 'x' and 'y' parameters.
 */
void key_pressed(unsigned char key, int x, int y)
{
    /* If 'q' is pressed, quit the program.
     */
    if (key == 'q')
    {
        exit(0);
    }
    /* If 't' is pressed, toggle our 'wireframe_mode' boolean to make OpenGL
     * render our cubes as surfaces of wireframes.
     */
    else if (key == 't')
    {
        wireframe_mode = !wireframe_mode;
        /* Tell OpenGL that it needs to re-render our scene with the cubes
         * now as wireframes (or surfaces if they were wireframes before).
         */
        glutPostRedisplay();
    }
    else
    {
        /* These might look a bit complicated, but all we are really doing is
         * using our current change in the horizontal camera angle (ie. the
         * value of 'x_view_angle') to compute the correct changes in our x and
         * z coordinates in camera space as we move forward, backward, to the left,
         * or to the right.
         *
         * 'step_size' is an arbitrary value to determine how "big" our steps
         * are.
         *
         * We make the x and z coordinate changes to the camera position, since
         * moving forward, backward, etc is basically just shifting our view
         * of the scene.
         */
        float x_view_rad = deg2rad(x_view_angle);

        // Pressing the key starts the smoothing
        if (key == start_smoothing_key)
        {  
            glutTimerFunc(FRAME_RATE, smoothNextFrame, FRAME_RATE);
        }

        /* 'w' for step forward
         */
        else if(key == 'w')
        {
            cam_position[0] += step_size * sin(x_view_rad);
            cam_position[2] -= step_size * cos(x_view_rad);
            glutPostRedisplay();
        }
        /* 'a' for step left
         */
        else if(key == 'a')
        {
            cam_position[0] -= step_size * cos(x_view_rad);
            cam_position[2] -= step_size * sin(x_view_rad);
            glutPostRedisplay();
        }
        /* 's' for step backward
         */
        else if(key == 's')
        {
            cam_position[0] -= step_size * sin(x_view_rad);
            cam_position[2] += step_size * cos(x_view_rad);
            glutPostRedisplay();
        }
        /* 'd' for step right
         */
        else if(key == 'd')
        {
            cam_position[0] += step_size * cos(x_view_rad);
            cam_position[2] += step_size * sin(x_view_rad);
            glutPostRedisplay();
        }
    }
}


// Frees all heap allocated data for the program
void destroy_objects() {
    for (map<string, Object>::iterator obj_iter = objects.begin(); 
                                    obj_iter != objects.end(); obj_iter++) {
        Object &obj = objects[obj_iter->first];

        delete_HE(obj.hevs, obj.hefs);
    }
}


void usage(void) {
    cerr << "usage: scene_description_file.txt xres yres h\n\t"
            "xres, yres (screen resolution) must be positive integers\n\t"
            "h (smoothing time step) must be a positive float (large values may cause error)\n";
    exit(1);
}


/* The 'main' function:
 *
 * This function is short, but is basically where everything comes together.
 */
int main(int argc, char* argv[])
{
    /* Checks that the user inputted the right parameters into the command line
     * and stores xres, yres, and filename to their respective fields
     */
    if (argc != 5) {
        usage();
    }
    int xres = stoi(argv[2]);
    int yres = stoi(argv[3]);
    time_step_h = stof(argv[4]);
    if (xres <= 0 || yres <= 0 || time_step_h <= 0) {
        usage();
    }

    /* 'glutInit' intializes the GLUT (Graphics Library Utility Toolkit) library.
     * This is necessary, since a lot of the functions we used above and below
     * are from the GLUT library.
     *
     * 'glutInit' takes the 'main' function arguments as parameters. This is not
     * too important for us, but it is possible to give command line specifications
     * to 'glutInit' by putting them with the 'main' function arguments.
     */
    glutInit(&argc, argv);
    /* The following line of code tells OpenGL that we need a double buffer,
     * a RGB pixel buffer, and a depth buffer.
     */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    /* The following line tells OpenGL to create a program window of size
     * 'xres' by 'yres'.
     */
    glutInitWindowSize(xres, yres);
    /* The following line tells OpenGL to set the program window in the top-left
     * corner of the computer screen (0, 0).
     */
    glutInitWindowPosition(0, 0);
    /* The following line tells OpenGL to name the program window "Test".
     */
    glutCreateWindow("Assignment 5 - Geometry Processing");
    
    /* Call our 'init' function...
     */
    init(argv[1]);
    /* Specify to OpenGL our display function.
     */
    glutDisplayFunc(display);
    /* Specify to OpenGL our reshape function.
     */
    glutReshapeFunc(reshape);
    /* Specify to OpenGL our function for handling mouse presses.
     */
    glutMouseFunc(mouse_pressed);
    /* Specify to OpenGL our function for handling mouse movement.
     */
    glutMotionFunc(mouse_moved);
    /* Specify to OpenGL our function for handling key presses.
     */
    glutKeyboardFunc(key_pressed);
    /* The following line tells OpenGL to start the "event processing loop". This
     * is an infinite loop where OpenGL will continuously use our display, reshape,
     * mouse, and keyboard functions to essentially run our program.
     */
    glutMainLoop();
    /* Frees all the memory allocated for the objects that needs to be destructed */
    destroy_objects();
}
