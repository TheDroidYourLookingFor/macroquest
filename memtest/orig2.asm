        TITLE orig.asm
        .386P

EXTRN _myextern_array:DWORD

_TEXT   SEGMENT PARA USE32 PUBLIC 'CODE'

PUBLIC __MemChecker2

__MemChecker2   proc near               ; CODE XREF: sub_55A8BB+15p
                                        ; sub_55A8BB+2Bbp ...

arg_0           = dword ptr  8
arg_4           = dword ptr  0Ch
arg_8           = byte ptr  10h
arg_9           = byte ptr  11h
arg_A           = byte ptr  12h
arg_B           = byte ptr  13h

                push    ebp
                mov     ebp, esp
                xor     eax, eax
                mov     al, [ebp+arg_8]
                xor     edx, edx
                mov     dl, [ebp+arg_9]
                push    esi
                mov     esi, 0FFh
                mov     ecx, 0FFFFFFh
                push    edi
                not     eax
                and     eax, esi
                mov     eax, _myextern_array[eax*4]
                xor     eax, ecx
                xor     edx, eax
                and     edx, esi
                sar     eax, 8
                and     eax, ecx
                xor     eax, _myextern_array[edx*4]
                xor     edx, edx
                mov     dl, [ebp+arg_A]
                mov     edi, [ebp+arg_4]
                xor     edx, eax
                sar     eax, 8
                and     edx, esi
                and     eax, ecx
                xor     eax, _myextern_array[edx*4]
                xor     edx, edx
                mov     dl, [ebp+arg_B]
                xor     edx, eax
                sar     eax, 8
                and     edx, esi
                and     eax, ecx
                xor     eax, _myextern_array[edx*4]
                mov     edx, [ebp+arg_0]
                add     edi, edx
                cmp     edx, edi
                jnb     short loc_559F7A
                push    ebx

loc_559F60:                             ; CODE XREF: __MemChecker2+84j
                xor     ebx, ebx
                mov     bl, [edx]
                xor     ebx, eax
                sar     eax, 8
                and     ebx, esi
                and     eax, ecx
                xor     eax, _myextern_array[ebx*4]
                inc     edx
                cmp     edx, edi
                jb      short loc_559F60
                pop     ebx

loc_559F7A:                             ; CODE XREF: __MemChecker2+6Aj
                pop     edi
                not     eax
                pop     esi
                pop     ebp
                retn
__MemChecker2   endp


_TEXT   ENDS
END
