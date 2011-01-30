#include "ShadowWrapper.h"
#include "BoundsCalculator.h"

static long double FLOAT_MIN_VAL = -3.4028235e38;
static long double FLOAT_MAX_VAL = 3.4028235e38;

UInt32 __declspec(naked) sub_705450(void *Arg1, void *Arg2, UInt32 Arg3, void *Arg4, void *Arg5)
{
	__asm
	{
		mov     edx, [esp+8]
		mov     eax, [esp+4]
		push    ebx
		push    ebp
		mov     ebp, [esp+1Ch]
		push    esi
		xor     ebx, ebx
		cmp     dword ptr [esp+18h], 4
		push    edi
		mov     edi, [esp+20h]
		jl      loc_7055EB
		lea     ecx, [edi+4]
		lea     esi, [ebp+0Ch]
		sub     ebp, edi
		mov     edi, [esp+1Ch]
		add     edi, 0FFFFFFFCh
		shr     edi, 2
		add     edi, 1
		lea     ebx, ds:0[edi*4]
		lea     ecx, [ecx+0]

loc_705490:                          
		fld     dword ptr [eax]
		fmul    dword ptr [ecx-4]
		fadd    dword ptr [edx]
		fld     dword ptr [eax+4]
		fmul    dword ptr [ecx]
		faddp   st(1), st
		fld     dword ptr [ecx+4]
		fmul    dword ptr [eax+8]
		faddp   st(1), st
		fstp    dword ptr [esi-0Ch]
		fld     dword ptr [eax+0Ch]
		fmul    dword ptr [ecx-4]
		fadd    dword ptr [edx+4]
		fld     dword ptr [eax+10h]
		fmul    dword ptr [ecx]
		faddp   st(1), st
		fld     dword ptr [eax+14h]
		fmul    dword ptr [ecx+4]
		faddp   st(1), st
		fstp    dword ptr [ecx+ebp]
		fld     dword ptr [eax+18h]
		fmul    dword ptr [ecx-4]
		fadd    dword ptr [edx+8]
		fld     dword ptr [ecx]
		fmul    dword ptr [eax+1Ch]
		faddp   st(1), st
		fld     dword ptr [eax+20h]
		fmul    dword ptr [ecx+4]
		faddp   st(1), st
		fstp    dword ptr [esi-4]
		fld     dword ptr [eax]
		fmul    dword ptr [ecx+8]
		fadd    dword ptr [edx]
		fld     dword ptr [eax+4]
		fmul    dword ptr [ecx+0Ch]
		faddp   st(1), st
		fld     dword ptr [ecx+10h]
		fmul    dword ptr [eax+8]
		faddp   st(1), st
		fstp    dword ptr [esi]
		fld     dword ptr [eax+0Ch]
		fmul    dword ptr [ecx+8]
		fadd    dword ptr [edx+4]
		fld     dword ptr [eax+10h]
		fmul    dword ptr [ecx+0Ch]
		faddp   st(1), st
		fld     dword ptr [eax+14h]
		fmul    dword ptr [ecx+10h]
		faddp   st(1), st
		fstp    dword ptr [esi+4]
		fld     dword ptr [eax+18h]
		fmul    dword ptr [ecx+8]
		fadd    dword ptr [edx+8]
		fld     dword ptr [ecx+0Ch]
		fmul    dword ptr [eax+1Ch]
		faddp   st(1), st
		fld     dword ptr [eax+20h]
		fmul    dword ptr [ecx+10h]
		faddp   st(1), st
		fstp    dword ptr [esi+8]
		fld     dword ptr [eax]
		fmul    dword ptr [ecx+14h]
		fadd    dword ptr [edx]
		fld     dword ptr [eax+4]
		fmul    dword ptr [ecx+18h]
		faddp   st(1), st
		fld     dword ptr [ecx+1Ch]
		fmul    dword ptr [eax+8]
		faddp   st(1), st
		fstp    dword ptr [esi+0Ch]
		fld     dword ptr [eax+0Ch]
		fmul    dword ptr [ecx+14h]
		fadd    dword ptr [edx+4]
		fld     dword ptr [eax+10h]
		fmul    dword ptr [ecx+18h]
		faddp   st(1), st
		fld     dword ptr [eax+14h]
		fmul    dword ptr [ecx+1Ch]
		faddp   st(1), st
		fstp    dword ptr [esi+10h]
		fld     dword ptr [eax+18h]
		fmul    dword ptr [ecx+14h]
		add     ecx, 30h
		add     esi, 30h
		sub     edi, 1
		fadd    dword ptr [edx+8]
		fld     dword ptr [ecx-18h]
		fmul    dword ptr [eax+1Ch]
		faddp   st(1), st
		fld     dword ptr [eax+20h]
		fmul    dword ptr [ecx-14h]
		faddp   st(1), st
		fstp    dword ptr [esi-1Ch]
		fld     dword ptr [eax]
		fmul    dword ptr [ecx-10h]
		fadd    dword ptr [edx]
		fld     dword ptr [eax+4]
		fmul    dword ptr [ecx-0Ch]
		faddp   st(1), st
		fld     dword ptr [ecx-8]
		fmul    dword ptr [eax+8]
		faddp   st(1), st
		fstp    dword ptr [esi-18h]
		fld     dword ptr [eax+0Ch]
		fmul    dword ptr [ecx-10h]
		fadd    dword ptr [edx+4]
		fld     dword ptr [eax+10h]
		fmul    dword ptr [ecx-0Ch]
		faddp   st(1), st
		fld     dword ptr [eax+14h]
		fmul    dword ptr [ecx-8]
		faddp   st(1), st
		fstp    dword ptr [esi-14h]
		fld     dword ptr [eax+18h]
		fmul    dword ptr [ecx-10h]
		fadd    dword ptr [edx+8]
		fld     dword ptr [ecx-0Ch]
		fmul    dword ptr [eax+1Ch]
		faddp   st(1), st
		fld     dword ptr [eax+20h]
		fmul    dword ptr [ecx-8]
		faddp   st(1), st
		fstp    dword ptr [esi-10h]
		jnz     loc_705490
		mov     ebp, [esp+24h]
		mov     edi, [esp+20h]

loc_7055EB:                            
		cmp     ebx, [esp+1Ch]
		jnb     loc_705665
		lea     ecx, [ebx+ebx*2]
		add     ecx, ecx
		add     ecx, ecx
		lea     esi, [ecx+ebp]
		lea     ecx, [ecx+edi+4]
		sub     ebp, edi
		mov     edi, [esp+1Ch]
		sub     edi, ebx

		loc_705607:
		fld     dword ptr [eax]
		add     ecx, 0Ch
		fmul    dword ptr [ecx-10h]
		add     esi, 0Ch
		sub     edi, 1
		fadd    dword ptr [edx]
		fld     dword ptr [eax+4]
		fmul    dword ptr [ecx-0Ch]
		faddp   st(1), st
		fld     dword ptr [ecx-8]
		fmul    dword ptr [eax+8]
		faddp   st(1), st
		fstp    dword ptr [esi-0Ch]
		fld     dword ptr [eax+0Ch]
		fmul    dword ptr [ecx-10h]
		fadd    dword ptr [edx+4]
		fld     dword ptr [eax+10h]
		fmul    dword ptr [ecx-0Ch]
		faddp   st(1), st
		fld     dword ptr [eax+14h]
		fmul    dword ptr [ecx-8]
		faddp   st(1), st
		fstp    dword ptr [ecx+ebp-0Ch]
		fld     dword ptr [eax+18h]
		fmul    dword ptr [ecx-10h]
		fadd    dword ptr [edx+8]
		fld     dword ptr [ecx-0Ch]
		fmul    dword ptr [eax+1Ch]
		faddp   st(1), st
		fld     dword ptr [eax+20h]
		fmul    dword ptr [ecx-8]
		faddp   st(1), st
		fstp    dword ptr [esi-4]
		jnz     loc_705607

loc_705665:
		pop     edi
		pop     esi
		pop     ebp
		pop     ebx
		retn
	}
}
UInt32 __declspec(naked) sub_704F70(void* Arg1, void* Arg2)
{
	__asm
	{
		mov     edx, [esp+8]
		fld     dword ptr [edx]
		mov     eax, [esp+4]
		fmul    dword ptr [ecx]
		fld     dword ptr [edx+0Ch]
		fmul    dword ptr [ecx+4]
		faddp   st(1), st
		fld     dword ptr [edx+18h]
		fmul    dword ptr [ecx+8]
		faddp   st(1), st
		fstp    dword ptr [eax]
		fld     dword ptr [ecx+10h]
		fmul    dword ptr [edx+0Ch]
		fld     dword ptr [ecx+0Ch]
		fmul    dword ptr [edx]
		faddp   st(1), st
		fld     dword ptr [edx+18h]
		fmul    dword ptr [ecx+14h]
		faddp   st(1), st
		fstp    dword ptr [eax+0Ch]
		fld     dword ptr [ecx+1Ch]
		fmul    dword ptr [edx+0Ch]
		fld     dword ptr [ecx+18h]
		fmul    dword ptr [edx]
		faddp   st(1), st
		fld     dword ptr [edx+18h]
		fmul    dword ptr [ecx+20h]
		faddp   st(1), st
		fstp    dword ptr [eax+18h]
		fld     dword ptr [edx+4]
		fmul    dword ptr [ecx]
		fld     dword ptr [edx+10h]
		fmul    dword ptr [ecx+4]
		faddp   st(1), st
		fld     dword ptr [ecx+8]
		fmul    dword ptr [edx+1Ch]
		faddp   st(1), st
		fstp    dword ptr [eax+4]
		fld     dword ptr [edx+10h]
		fmul    dword ptr [ecx+10h]
		fld     dword ptr [edx+4]
		fmul    dword ptr [ecx+0Ch]
		faddp   st(1), st
		fld     dword ptr [edx+1Ch]
		fmul    dword ptr [ecx+14h]
		faddp   st(1), st
		fstp    dword ptr [eax+10h]
		fld     dword ptr [edx+10h]
		fmul    dword ptr [ecx+1Ch]
		fld     dword ptr [ecx+18h]
		fmul    dword ptr [edx+4]
		faddp   st(1), st
		fld     dword ptr [edx+1Ch]
		fmul    dword ptr [ecx+20h]
		faddp   st(1), st
		fstp    dword ptr [eax+1Ch]
		fld     dword ptr [edx+8]
		fmul    dword ptr [ecx]
		fld     dword ptr [edx+14h]
		fmul    dword ptr [ecx+4]
		faddp   st(1), st
		fld     dword ptr [ecx+8]
		fmul    dword ptr [edx+20h]
		faddp   st(1), st
		fstp    dword ptr [eax+8]
		fld     dword ptr [edx+14h]
		fmul    dword ptr [ecx+10h]
		fld     dword ptr [edx+8]
		fmul    dword ptr [ecx+0Ch]
		faddp   st(1), st
		fld     dword ptr [edx+20h]
		fmul    dword ptr [ecx+14h]
		faddp   st(1), st
		fstp    dword ptr [eax+14h]
		fld     dword ptr [edx+14h]
		fmul    dword ptr [ecx+1Ch]
		fld     dword ptr [ecx+18h]
		fmul    dword ptr [edx+8]
		faddp   st(1), st
		fld     dword ptr [edx+20h]
		fmul    dword ptr [ecx+20h]
		faddp   st(1), st
		fstp    dword ptr [eax+20h]
		retn    8
	}
}
UInt32 __declspec(naked) sub_7050C0(void* Arg1, void* Arg2)
{
	__asm
	{
		mov     edx, [esp+8]
		fld     dword ptr [ecx+4]
		fmul    dword ptr [edx+4]
		mov     eax, [esp+4]
		fld     dword ptr [edx]
		fmul    dword ptr [ecx]
		faddp   st(1), st
		fld     dword ptr [ecx+8]
		fmul    dword ptr [edx+8]
		faddp   st(1), st
		fstp    dword ptr [eax]
		fld     dword ptr [ecx+0Ch]
		fmul    dword ptr [edx]
		fld     dword ptr [ecx+10h]
		fmul    dword ptr [edx+4]
		faddp   st(1), st
		fld     dword ptr [ecx+14h]
		fmul    dword ptr [edx+8]
		faddp   st(1), st
		fstp    dword ptr [eax+4]
		fld     dword ptr [ecx+18h]
		fmul    dword ptr [edx]
		fld     dword ptr [ecx+1Ch]
		fmul    dword ptr [edx+4]
		faddp   st(1), st
		fld     dword ptr [ecx+20h]
		fmul    dword ptr [edx+8]
		faddp   st(1), st
		fstp    dword ptr [eax+8]
		retn    8
	}
}




/**************************************************/
void __declspec(naked) CalculateBoundsForNiNode(void *Arg1, void *Arg2, void *Arg3, void *Arg4, void *Arg5, void *Arg6)
{
	__asm
	{
		sub     esp, 64h
		push    esi
		mov     esi, [esp+6Ch]
		test    esi, esi
		jz      loc_512EF5
		mov     eax, [esi]
		mov     edx, [eax+8]
		push    ebx
		mov     ecx, esi
		call    edx
		mov     ebx, eax
		test    ebx, ebx
		jz      loc_512C97
		push    edi
		movzx   edi, word ptr [ebx+0B6h]
		test    edi, edi
		jz      loc_512C90
		push    ebp
		mov     ebp, [esp+84h]
		lea     esp, [esp+0]
		loc_512BA0:                
		movzx   eax, word ptr [ebx+0B6h]
		sub     edi, 1
		cmp     eax, edi
		jbe     loc_512C87
		mov     ecx, [ebx+0B0h]
		mov     esi, [ecx+edi*4]
		test    esi, esi
		jz      loc_512C87
		mov     ecx, [esp+88h]
		lea     edx, [esi+30h]
		push    edx
		lea     eax, [esp+54h]
		push    eax
		call    sub_704F70
		fld     dword ptr [esi+60h]
		mov     ecx, [esp+8Ch]
		fmul    dword ptr [ecx]
		mov     ecx, [esp+88h]
		add     esi, 54h
		push    esi
		lea     edx, [esp+3Ch]
		fstp    dword ptr [esp+14h]
		push    edx
		call    sub_7050C0
		mov     ecx, [esp+8Ch]
		fld     dword ptr [ecx]
		movzx   edx, word ptr [ebx+0B6h]
		cmp     edx, edi
		fstp    dword ptr [esp+78h]
		fld     dword ptr [esp+78h]
		fld     dword ptr [eax]
		fmul    st, st(1)
		fstp    dword ptr [esp+14h]
		fld     dword ptr [eax+4]
		fmul    st, st(1)
		fstp    dword ptr [esp+18h]
		fmul    dword ptr [eax+8]
		fstp    dword ptr [esp+1Ch]
		fld     dword ptr [ebp+0]
		fadd    dword ptr [esp+14h]
		fstp    dword ptr [esp+20h]
		fld     dword ptr [ebp+4]
		fadd    dword ptr [esp+18h]
		fstp    dword ptr [esp+24h]
		fld     dword ptr [ebp+8]
		fadd    dword ptr [esp+1Ch]
		fstp    dword ptr [esp+28h]
		ja      loc_512C56
		xor     eax, eax
		jmp     loc_512C5F

		loc_512C56:                             
		mov     eax, [ebx+0B0h]
		mov     eax, [eax+edi*4]
		loc_512C5F:                         
		lea     ecx, [esp+10h]
		push    ecx
		lea     edx, [esp+54h]
		push    edx
		mov     edx, [esp+88h]
		lea     ecx, [esp+28h]
		push    ecx
		mov     ecx, [esp+88h]
		push    edx
		push    ecx
		push    eax
		call    CalculateBoundsForNiNode
		add     esp, 18h
		loc_512C87:                            
								
		test    edi, edi
		jnz     loc_512BA0
		pop     ebp
		loc_512C90:                             
		pop     edi
		pop     ebx
		pop     esi
		add     esp, 64h
		retn

		loc_512C97:                             
		mov     edx, [esi]
		mov     eax, [edx+10h]
		mov     ecx, esi
		call    eax
		test    eax, eax
		jz      loc_512EF4
		mov     eax, [esi+0B4h]
		fld     ds:FLOAT_MAX_VAL
		movzx   edx, word ptr [eax+8]
		test    edx, edx
		mov     ecx, [eax+1Ch]
		fst     dword ptr [esp+30h]
		fst     dword ptr [esp+34h]
		fstp    dword ptr [esp+38h]
		fld     ds:FLOAT_MIN_VAL
		fst     dword ptr [esp+3Ch]
		fst     dword ptr [esp+40h]
		fstp    dword ptr [esp+44h]
		jz      loc_512D75
		loc_512CE1:                             
		fld     dword ptr [ecx]
		sub     edx, 1
		fld     dword ptr [esp+30h]
		fcompp
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512CF9
		fld     dword ptr [ecx]
		fstp    dword ptr [esp+30h]
		loc_512CF9:                             
		fld     dword ptr [ecx]
		fld     dword ptr [esp+3Ch]
		fcompp
		fnstsw  ax
		test    ah, 5
		jp      loc_512D0E
		fld     dword ptr [ecx]
		fstp    dword ptr [esp+3Ch]
		loc_512D0E:                             
		fld     dword ptr [ecx+4]
		fld     dword ptr [esp+34h]
		fcompp
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512D25
		fld     dword ptr [ecx+4]
		fstp    dword ptr [esp+34h]
		loc_512D25:                             
		fld     dword ptr [ecx+4]
		fld     dword ptr [esp+40h]
		fcompp
		fnstsw  ax
		test    ah, 5
		jp      loc_512D3C
		fld     dword ptr [ecx+4]
		fstp    dword ptr [esp+40h]
		loc_512D3C:                             
		fld     dword ptr [ecx+8]
		fld     dword ptr [esp+38h]
		fcompp
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512D53
		fld     dword ptr [ecx+8]
		fstp    dword ptr [esp+38h]
		loc_512D53:                            
		fld     dword ptr [ecx+8]
		fld     dword ptr [esp+44h]
		fcompp
		fnstsw  ax
		test    ah, 5
		jp      loc_512D6A
		fld     dword ptr [ecx+8]
		fstp    dword ptr [esp+44h]
		loc_512D6A:                             
		add     ecx, 0Ch
		test    edx, edx
		jnz     loc_512CE1
		loc_512D75:                             
		mov     eax, [esp+7Ch]
		lea     ecx, [esp+18h]
		push    ecx
		mov     ecx, [esp+84h]
		lea     edx, [esp+34h]
		push    edx
		push    2
		push    eax
		push    ecx
		call    sub_705450
		fld     dword ptr [esp+38h]
		mov     eax, [esp+2Ch] 
		fld     st
		fld     dword ptr [esp+2Ch]
		mov     ecx, [esp+30h]
		mov     edx, [esp+34h]
		fcom    st(1)
		mov     [esp+50h], eax
		mov     [esp+44h], eax
		add     esp, 14h
		fnstsw  ax
		mov     [esp+40h], ecx
		mov     [esp+44h], edx
		test    ah, 41h
		mov     [esp+34h], ecx
		mov     [esp+38h], edx
		jnz     loc_512DD5
		fxch    st(2)
		fst     dword ptr [esp+30h]
		fxch    st(2)
		loc_512DD5:                            
		fcompp
		fnstsw  ax
		test    ah, 5
		jp      loc_512DE4
		fstp    dword ptr [esp+3Ch]
		jmp     loc_512DE6

		loc_512DE4:                            
		fstp    st
		loc_512DE6:                            
		fld     dword ptr [esp+28h]
		fld     st
		fld     dword ptr [esp+34h]
		fcom    st(1)
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512E0B
		fstp    st
		fxch    st(1)
		fst     dword ptr [esp+34h]
		fld     dword ptr [esp+34h]
		fxch    st(1)
		fxch    st(2)
		fxch    st(1)
		loc_512E0B:                             
		fld     dword ptr [esp+40h]
		fcom    st(2)
		fnstsw  ax
		fstp    st(2)
		test    ah, 5
		jp      loc_512E28
		fstp    st(1)
		fxch    st(1)
		fstp    dword ptr [esp+40h]
		fld     dword ptr [esp+40h]
		jmp     loc_512E2A

		loc_512E28:                           
		fstp    st(2)
		loc_512E2A:                            
		fld     dword ptr [esp+2Ch]
		fld     st
		fld     dword ptr [esp+38h]
		fcom    st(1)
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512E4F
		fstp    st
		fxch    st(1)
		fst     dword ptr [esp+38h]
		fld     dword ptr [esp+38h]
		fxch    st(1)
		fxch    st(2)
		fxch    st(1)
		loc_512E4F:                            
		fld     dword ptr [esp+44h]
		fcom    st(2)
		fnstsw  ax
		fstp    st(2)
		test    ah, 5
		jp      loc_512E6C
		fstp    st(1)
		fxch    st(1)
		fstp    dword ptr [esp+44h]
		fld     dword ptr [esp+44h]
		jmp     loc_512E6E

		loc_512E6C:                             
		fstp    st(2)
		loc_512E6E:                             
		fld     dword ptr [esp+30h]
		mov     edx, [esp+74h]
		fld     dword ptr [edx]
		fcomp   st(1)
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512E85
		fstp    dword ptr [edx]
		jmp     loc_512E87

		loc_512E85:                             
		fstp    st
		loc_512E87:                            
		fld     dword ptr [esp+3Ch]
		mov     ecx, [esp+78h]
		fld     dword ptr [ecx]
		fcomp   st(1)
		fnstsw  ax
		test    ah, 5
		jp      loc_512E9E
		fstp    dword ptr [ecx]
		jmp     loc_512EA0

		loc_512E9E:                            
		fstp    st
		loc_512EA0:                            
		fld     dword ptr [edx+4]
		fcomp   st(4)
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512EB3
		fxch    st(3)
		fstp    dword ptr [edx+4]
		jmp     loc_512EB5

		loc_512EB3:                            
		fstp    st(3)
		loc_512EB5:                            
		fld     dword ptr [ecx+4]
		fcomp   st(2)
		fnstsw  ax
		test    ah, 5
		jp      loc_512EC8
		fxch    st(1)
		fstp    dword ptr [ecx+4]
		jmp     loc_512ECA

		loc_512EC8:                             
		fstp    st(1)
		loc_512ECA:                            
		fld     dword ptr [edx+8]
		fcomp   st(1)
		fnstsw  ax
		test    ah, 41h
		jnz     loc_512EDB
		fstp    dword ptr [edx+8]
		jmp     loc_512EDD

		loc_512EDB:                            
		fstp    st
		loc_512EDD:                             
		fld     dword ptr [ecx+8]
		fcomp   st(1)
		fnstsw  ax
		test    ah, 5
		jp      loc_512EF2
		pop     ebx
		fstp    dword ptr [ecx+8]
		pop     esi
		add     esp, 64h
		retn

		loc_512EF2:                             
		fstp    st
		loc_512EF4:                            
		pop     ebx
		loc_512EF5:                            
		pop     esi
		add     esp, 64h
		retn
    }
}