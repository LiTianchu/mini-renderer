#ifndef __HE_MODEL_H__
#define __HE_MODEL_H__

#include "geometry.h"
#include <vector>
#include <set>
#include <string>
#include "tgaimage.h"

struct Face; // forward declare to work around cyclic dependency
struct Vertex;

struct HEdge
{
	HEdge *pair; // oppositely oriented half-edge
	HEdge *next; // next half-edge around the face
	HEdge *prev; // previous half-edge in the face
	Vertex *v;	 // vertex at the end of the half-edge
	Face *f;	 // face the half-edge borders
	int index;
	HEdge() : pair(), next(), v(), f() {}
	HEdge(HEdge *pair_, HEdge *next_, Vertex *v_, Face *f_) : pair(pair_), next(next_), v(v_), f(f_) {}
	inline bool operator<(const HEdge &he) const { return index < he.index; };
	inline bool operator==(const HEdge &he) const { return index == he.index; };
	inline bool operator>(const HEdge &he) const { return index > he.index; };
};

struct Edge //undirected edge used for checkings when loading data, not involved in the final data structure
{
	Vertex *v1; //vertices of the 2 ends, order does not matter
	Vertex *v2;
	HEdge *h; //one of the half edge belong to this undirected edge
};


struct Face
{
	HEdge *h; // one of the half-edges bordering the face
	int index;
	Face() : h() {}
	Face(HEdge *h_) : h(h_) {}
	inline bool operator<(const Face &f) const { return index < f.index; };
	inline bool operator==(const Face &f) const { return index == f.index; };
	inline bool operator>(const Face &f) const { return index > f.index; };
};

struct Vertex
{
	HEdge *h; // one of the half-edges points to this vertex
	Vec3f pos; //model space position
	Vec3f norm;
	Vec2f tex_coord;
	Vec3f pos_view; //view space position
	Vec3f pos_proj; //projection space position
	Vec2f pos_screen; //screen space position
	float screen_z; //screen space z value

	//for comparison
	int pos_index;
	int uv_index;
	Vertex() : h(), pos(), norm(), tex_coord() {}
	Vertex(HEdge *_h, Vec3f _pos, Vec3f _norm, Vec2f _tex_coord) : h(_h), pos(_pos), norm(_norm), tex_coord(_tex_coord) {}
	//uniqueness of vertices are determined by the position and uv, as one vertex can have diff uv on diff faces
	inline bool operator<(const Vertex &v) const {
        return (pos_index< v.pos_index) || ((pos_index == v.pos_index) && (uv_index < v.uv_index));
    }

    inline bool operator==(const Vertex &v) const {
        return (pos_index == v.pos_index) && (uv_index == v.uv_index);
    }

    inline bool operator>(const Vertex &v) const {
        return (pos_index > v.pos_index) || ((pos_index == v.pos_index) && (uv_index > v.uv_index));
    }
};

struct HEdge_Compare
{
	bool operator()(const HEdge *a, HEdge *b) const
	{
		return a->index < b->index;
	}
};

struct Face_Compare
{
	bool operator()(const Face *a, const Face *b) const
	{
		return a->index < b->index;
	}
};

struct Vertex_Compare
{
	bool operator()(const Vertex *a, const Vertex *b) const
	{
		//it will equal when the position index and uv index are the same
		return a->pos_index < b->pos_index || (a->pos_index == b->pos_index && a->uv_index < b->uv_index);
	}
};

class HEModel
{
private:
	std::set<HEdge *, HEdge_Compare> h_edges;
	std::set<Face *, Face_Compare> faces;
	std::set<Vertex *, Vertex_Compare> vertices;
	TGAImage *diffuse_texture;
	TGAImage *normal_map_texture;

public:
	HEModel(const char *filename);
	~HEModel();
	std::set<HEdge *>::iterator h_edges_begin() { return h_edges.begin(); }
	std::set<HEdge *>::iterator h_edges_end() { return h_edges.end(); }
	std::set<Face *>::iterator faces_begin() { return faces.begin(); }
	std::set<Face *>::iterator faces_end() { return faces.end(); }
	std::set<Vertex *>::iterator vertices_begin() { return vertices.begin(); }
	std::set<Vertex *>::iterator vertices_end() { return vertices.end(); }
	int num_of_h_edges() { return h_edges.size(); }
	int num_of_faces() { return faces.size(); }
	int num_of_vertices() { return vertices.size(); }
	void qem_simplify(int target_num_of_faces);
	void remove_degenerate_faces(Vertex& v);
	void remove_half_edge(HEdge *he){ h_edges.erase(he); delete he; }
	void remove_face(Face *f){ faces.erase(f); delete f; }
	void remove_vertex(Vertex *v){ vertices.erase(v); delete v; }
	TGAImage *get_diffuse_texture() { return diffuse_texture; }
	TGAImage *get_normal_map_texture() { return normal_map_texture; }
	void set_diffuse_texture(TGAImage *diffuse_texture_) { diffuse_texture = diffuse_texture_; }
	void set_normal_map_texture(TGAImage *normal_map_texture_) { normal_map_texture = normal_map_texture_; }

};

#endif