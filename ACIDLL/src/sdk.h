#pragma once

#include <cmath>
#include <span>
#include <string_view>

namespace SDK {
    enum { GUN_KNIFE = 0, GUN_PISTOL, GUN_CARBINE, GUN_SHOTGUN, GUN_SUBGUN, GUN_SNIPER, GUN_ASSAULT, GUN_GRENADE, GUN_AKIMBO, NUMGUNS };
#define valid_weapon(g) ((g) >= GUN_KNIFE && (g) < NUMGUNS)
#define reloadable_gun(g) ((g) != GUN_KNIFE && (g) != GUN_GRENADE)

#define isteam(a,b)   (m_teammode && (a) == (b))
#define team_opposite(o) (team_isvalid(o) && (o) < TEAM_SPECT ? (o) ^ 1 : TEAM_SPECT)
#define team_base(t) ((t) & 1)
#define team_basestring(t) ((t) == 1 ? SDK::G::teamnames[1] : ((t) == 0 ? SDK::G::teamnames[0] : "SPECT"))
#define team_isvalid(t) ((int(t)) >= 0 && (t) < TEAM_NUM)
#define team_isactive(t) ((t) == TEAM_CLA || (t) == TEAM_RVSF)
#define team_isspect(t) ((t) > TEAM_RVSF && (t) < TEAM_NUM)
#define team_group(t) ((t) == TEAM_SPECT ? TEAM_SPECT : team_base(t))
#define team_tospec(t) ((t) == TEAM_SPECT ? TEAM_SPECT : team_base(t) + TEAM_CLA_SPECT - TEAM_CLA)

#define MAXSTRLEN 260
    struct acString {
        operator std::string_view() const { return std::string_view(data_); }
        acString& operator=(std::string_view str) {
            size_t len = str.copy(data_, MAXSTRLEN - 1);
            data_[len] = '\0';
            return *this;
        }
        const char* c_str() const { return data_; }
        size_t size() const { return strlen(data_); }
		bool empty() const { return data_[0] == '\0'; }

		char data_[MAXSTRLEN];
    };

    struct Colour {
        union {
            struct { float b, g, r, a; };
			float c[4];
		};
    };

    enum { ENT_PLAYER = 0, ENT_BOT, ENT_CAMERA, ENT_BOUNCE };
    enum { CS_ALIVE = 0, CS_DEAD, CS_SPAWNING, CS_LAGGED, CS_EDITING, CS_SPECTATE };
    enum { CR_DEFAULT = 0, CR_MASTER, CR_ADMIN, CR_OWNER, CR_NUM };
    enum { SM_NONE = 0, SM_DEATHCAM, SM_FOLLOW1ST, SM_FOLLOW3RD, SM_FOLLOW3RD_TRANSPARENT, SM_FLY, SM_OVERVIEW, SM_NUM };
    enum { FPCN_VOID = -4, FPCN_DEATHCAM = -2, FPCN_FLY = -2, FPCN_OVERVIEW = -1 };

    #define POSHIST_SIZE 7

    struct Vec3 {
        union {
            struct {
                float x, y, z;
            };
            float v[3];
        };

        operator float* () { return v; }
        operator const float* () const { return v; }

        Vec3() : x(0), y(0), z(0) {}
        Vec3(float x, float y, float z) : x(x), y(y), z(z) {}
        Vec3 operator+(const Vec3& other) const {
            return Vec3(x + other.x, y + other.y, z + other.z);
        }
        Vec3 operator-(const Vec3& other) const {
            return Vec3(x - other.x, y - other.y, z - other.z);
        }
        Vec3 operator*(float scalar) const {
            return Vec3(x * scalar, y * scalar, z * scalar);
        }
        Vec3& operator+=(const Vec3& other) {
            x += other.x; y += other.y; z += other.z;
            return *this;
        }
        Vec3& operator-=(const Vec3& other) {
            x -= other.x; y -= other.y; z -= other.z;
            return *this;
        }
        Vec3& operator*=(float scalar) {
            x *= scalar; y *= scalar; z *= scalar;
            return *this;
        }
        float length() const {
            return sqrtf(x * x + y * y + z * z);
        }
        float dist(Vec3 other) const {
            return (*this - other).length();
		}
    };

    struct Vec2 {
        float x, y;
    };

    class Mat4 {
    public:
        float m[4][4];
        operator float* () { return &m[0][0]; }
        operator const float* () const { return &m[0][0]; }

        Mat4 operator*(const Mat4& other) const {
            Mat4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i][j] = 0;
                    for (int k = 0; k < 4; ++k) {
                        result.m[i][j] += m[i][k] * other.m[k][j];
                    }
                }
            }
            return result;
        }
        Mat4 & operator*=(const Mat4& other) {
            *this = *this * other;
            return *this;
		}
        Mat4 transpose() const {
            Mat4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i][j] = m[j][i];
                }
            }
            return result;
		}
	};

	template <class T>
    class acVector {
    public:
		operator std::span<T>() const { return std::span<T>(data_, usedLen_); }
		std::span<T> span() const { return std::span<T>(data_, usedLen_); }

		T* data() { return data_; }
		int size() const { return usedLen_; }

    private:
        T* data_;
        int allocatedLen_;
        int usedLen_;
    };

    class PhysicsEntity {
    public:
        Vec3 origin, velocity;
        Vec3 deltaPos, newPos;
        float yaw, pitch, roll;
        float pitchVel;
        float maxSpeed;
        int timeInAir;
        float radius, eyeHeight, maxEyeHeight, aboveEye;
        bool inWater;
        bool onFloor, onLadder, jumpNext, jumped, crouching, crouchedInAir, tryCrouch, canCollide, stuck, scoping;
        int lastJump;
        float lastJumpHeight;
        int lastSplash;
        char move, strafe;
        unsigned char state, type;
        float eyeHeightVel;
        int lastPos;

        virtual ~PhysicsEntity() {}
        virtual void onCollision() {}
        virtual void onMoved(const Vec3& dist) {}
    };

    struct AnimationState {
        int anim, frame, range, basetime;
        float speed;

        bool operator==(const AnimationState& o) const { return frame == o.frame && range == o.range && basetime == o.basetime && speed == o.speed; }
        bool operator!=(const AnimationState& o) const { return frame != o.frame || range != o.range || basetime != o.basetime || speed != o.speed; }
    };

    class DynamicEntity : public PhysicsEntity {
    public:
        bool kLeft, kRight, kUp, kDown;

        AnimationState prev[2], current[2];
        int lastAnimSwitchTime[2];
        void* lastModel[2];
        int lastRendered;

        virtual ~DynamicEntity() {}
    };

    struct GunInfo {
        char modelName[23], title[42];
        short sound, reload, reloadTime;
        short attackdelay, damage, piercing;
        short projSpeed, part, spread;
        short recoil, magSize, mdlKickRot;
        short mdlKickBack, recoilIncrease, recoilBase;
        short maxRecoil, recoilBackFade, pushFactor;
        bool isAuto;

        static std::span<GunInfo> getAllGuns() {
            // 0x56CD20 == ptr to GunInfo guns[NUMGUNS]
            return std::span<GunInfo>((GunInfo*)0x56CD20, NUMGUNS);
        }
    };

    class PlayerState {
    public:
        int health, armour;
        int primary, nextprimary;
        int gunselect;
        bool akimbo;
        int ammo[NUMGUNS], mag[NUMGUNS], gunwait[NUMGUNS];
        int pstatshots[NUMGUNS], pstatdamage[NUMGUNS];

        virtual ~PlayerState() {}
    };

    struct PosHistory {
        int nextUpdate, curPos, numPos;
        Vec3 positions[POSHIST_SIZE];

        const int size() const { return numPos; }

        void reset() {
            curPos = 0;
            numPos = 0;
        }

        void addPos(const Vec3& o) {
            positions[curPos] = o;
            curPos++;
            if (curPos >= POSHIST_SIZE) curPos = 0;
            if (numPos < POSHIST_SIZE) numPos++;
        }

        const Vec3& getPos(int i) const {
            i = curPos - 1 - i;
            if (i < 0) i += POSHIST_SIZE;
            return positions[i];
        }

        void update(const Vec3& o, int lastMillis) {
            if (lastMillis < nextUpdate) return;
            if (o.dist(positions[0]) >= 4.0f) addPos(o);
            nextUpdate = lastMillis + 100;
        }
    };

    struct Weapon;
    class PlayerEntity : public DynamicEntity, public PlayerState {
    public:
        // 0x58BBEC = localPlayer
		static PlayerEntity* getLocalPlayer() { return *(PlayerEntity**)0x58BBEC; }
		static acVector<PlayerEntity*>* getPlayers() { return (acVector<PlayerEntity*>*)0x58BBF0; }

        int curSkin, nextSkin[2];
        int clientNum, lastUpdate, pLag, ping;
        uint32_t address;
        int lifeSequence;
        int frags, flagScore, deaths, tks;
        int lastAction, lastMove, lastPain, lastVoiceCom, lastDeath;
        int clientRole;
        bool attacking;
        acString name;
        int team;
        int weaponChanging;
        int nextWeapon; // weapon we switch to
        int spectateMode, followPlayerCN;
        int earDamageMillis;
        float maxRoll, maxRollEffect, movRoll, effRoll;  // roll added by movement and damage
        int ffov, scopeFov;
        bool allowMove() { return (state != CS_DEAD && state != CS_SPECTATE) || spectateMode == SM_FLY; }

        Weapon* weapons[NUMGUNS];
        Weapon* prevWeaponSel, * weaponSel, * nextWeaponSel, * primWeap, * nextPrimWeap, * lastAttackWeapon;

        PosHistory history; // Previous stored locations of this player. The bot uses this

        const char* skin_noteam, * skin_cla, * skin_rvsf;

        float deltaYaw, deltaPitch, newYaw, newPitch;
        int smoothmillis;

        Vec3 headPos;

        bool ignored, muted;
        bool nocorpse;

        virtual ~PlayerEntity() { /* dummy */ }

        bool isSpectating() { return state == CS_SPECTATE || (state == CS_DEAD && spectateMode > SM_NONE); }
        int skin(int t = -1) const { return nextSkin[team_base(t < 0 ? team : t)]; }
        void setSkin(int t, int s) {
            const int maxskin[2] = { 4, 6 };
            t = team_base(t < 0 ? team : t);
            nextSkin[t] = (s & 63) % maxskin[t];
        }

		std::span<Weapon*> getWeapons() { return std::span<Weapon*>(weapons, NUMGUNS); }
    };

    enum class BounceType : int { None, Nade, Gib };
    class BounceEntity : public PhysicsEntity {
    public:
        int millis, timeToLive;
        BounceType bounceType;
        float rotSpeed;
        bool plClipped;
        PlayerEntity* owner;

        virtual ~BounceEntity() {}

        bool isAlive(int lastmillis) const { return lastmillis - millis < timeToLive; }
        virtual void destroy() {}
        virtual bool applyPhysics() { return true; }
    };

    struct Weapon {
        virtual ~Weapon() {}

        int type;
        PlayerEntity* owner;
        const struct GunInfo& info;
        int& ammo, & mag, & gunwait, shots;
        virtual int dynSpread() = 0;
        virtual float dynRecoil() = 0;
        int reloading, lastAction;

        virtual bool attack(Vec3& targ) = 0;
        virtual void attackFX(const Vec3& from, const Vec3& to, int millis) = 0;
        virtual void attackPhysics(Vec3& from, Vec3& to) = 0;
        virtual void attackSound() = 0;
        virtual bool reload(bool autoreloaded) = 0;
        virtual void reset() {}
        virtual bool busy() { return false; }

        virtual int modelAnim() = 0;
        virtual void updateTimers(int millis) = 0;
        virtual bool selectable() = 0;
        virtual bool deselectable() = 0;
        virtual void renderStats() = 0;
        virtual void renderHUDModel() = 0;
        virtual void renderAimHelp(bool teamwarning) = 0;

        virtual void onSelecting(bool sound) = 0;
        virtual void onDeselecting() {}
        virtual void onAmmoPicked() {}
        virtual void onOwnerDies() {}
        virtual void removeBounceEntity(BounceEntity* b) {}

        virtual int flashTime() const = 0;
    };

    class GrenadeEntity;
    struct Grenades : Weapon {
        GrenadeEntity* inhandnade;
        const int throwwait;
        int throwmillis;
        int cookingmillis;
        int state;
    };

    struct Gun : Weapon {
        virtual bool attack(Vec3& targ) {}
        virtual void attackFX(const Vec3& from, const Vec3& to, int millis) {}
    };

    struct SubGun : Gun {};
    struct Carbine : Gun {};
    struct ShotGun : Gun {};
    struct AssaultRifle : Gun {};
    struct Pistol : Gun {};
    struct Knife : Weapon {};

    struct SniperRifle : Gun {
        bool scoped;
        int scoped_since;
    };

    struct Akimbo : Gun {
        int akimboside;
        int akimbomillis;
        int akimbolastaction[2];
    };
    
    // 0x4DC400 is conoutf
    constexpr uintptr_t CONOUTF_ABS = 0x4DC400;
    typedef void(__cdecl* conoutf_t)(const char* fmt, ...);
    inline conoutf_t conoutf = (conoutf_t)CONOUTF_ABS;

    constexpr uintptr_t LINE_ABS = 0x45CCE0;
	typedef void(__fastcall* line_t)(int, int, int, int, Colour*);
	inline line_t line = (line_t)LINE_ABS;

    namespace G {
        Mat4* mvpMatrix = (Mat4*)0x57EFB0;
		const char** teamnames = (const char**)0x56CD00;
        int* screenWidth = (int*)0x591E1C;
        int* screenHeight = (int*)0x592EC8;
	} // namespace G

    namespace Utils {
        bool W2S(const Vec3& pos, Vec2& out) {
            const auto m = G::mvpMatrix;

            float clipX = m->m[0][0] * pos.x + m->m[1][0] * pos.y + m->m[2][0] * pos.z + m->m[3][0];
            float clipY = m->m[0][1] * pos.x + m->m[1][1] * pos.y + m->m[2][1] * pos.z + m->m[3][1];
            float clipW = m->m[0][3] * pos.x + m->m[1][3] * pos.y + m->m[2][3] * pos.z + m->m[3][3];

            if (clipW < 0.01f)
                return false;

            float ndcX = clipX / clipW;
            float ndcY = clipY / clipW;

            out.x = (ndcX + 1.0f) * (*G::screenWidth) / 2.0f;
            out.y = (1.0f - ndcY) * (*G::screenHeight) / 2.0f;

            return true;
        }
    } // namespace Utils
} // namespace SDK
