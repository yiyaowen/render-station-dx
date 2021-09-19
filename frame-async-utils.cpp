/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
*/

#include "debugger.h"
#include "frame-async-utils.h"

void initEmptyFrameResource(D3DCore* pCore, FrameResource* pResource) {
    checkHR(pCore->device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&pResource->cmdAlloc)));
}

void initFResourceObjConstBuff(D3DCore* pCore, UINT objBuffCount, FrameResource* pResource) {
    createConstBuffPair(pCore, sizeof(ObjConsts), objBuffCount,
        &pResource->objConstBuffCPU, &pResource->objConstBuffGPU);
}

void initFResourceProcConstBuff(D3DCore* pCore, UINT procBuffCount, FrameResource* pResource) {
    createConstBuffPair(pCore, sizeof(ProcConsts), procBuffCount,
        &pResource->procConstBuffCPU, &pResource->procConstBuffGPU);
}

void initFResourceMatConstBuff(D3DCore* pCore, UINT matBuffCount, FrameResource* pResource) {
    createConstBuffPair(pCore, sizeof(MatConsts), matBuffCount,
        &pResource->matConstBuffCPU, &pResource->matConstBuffGPU);
}

void initEmptyRenderItem(RenderItem* pRitem) {
    pRitem->objConstBuffIdx = 0;

    ObjConsts constData;
    constData.worldTrans = makeIdentityFloat4x4();
    constData.invTrWorldTrans = makeIdentityFloat4x4();
    pRitem->constData = constData;
}

void drawRenderItems(D3DCore* pCore, RenderItem** ppRitem, UINT ritemCount) {
    for (UINT i = 0; i < ritemCount; ++i) {
        pCore->cmdList->IASetVertexBuffers(0, 1, &ppRitem[i]->mesh->vertexBuffView);
        pCore->cmdList->IASetIndexBuffer(&ppRitem[i]->mesh->indexBuffView);
        pCore->cmdList->IASetPrimitiveTopology(ppRitem[i]->topologyType);

        auto constBuffAddr = pCore->currFrameResource->objConstBuffGPU->GetGPUVirtualAddress();
        constBuffAddr += ppRitem[i]->objConstBuffIdx * calcConstBuffSize(sizeof(ObjConsts));
        pCore->cmdList->SetGraphicsRootConstantBufferView(0, constBuffAddr);

        constBuffAddr = pCore->currFrameResource->matConstBuffGPU->GetGPUVirtualAddress();
        constBuffAddr += ppRitem[i]->material->matConstBuffIdx * calcConstBuffSize(sizeof(MatConsts));
        pCore->cmdList->SetGraphicsRootConstantBufferView(2, constBuffAddr);

        CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(pCore->srvDescHeap->GetGPUDescriptorHandleForHeapStart());
        texHandle.Offset(ppRitem[i]->material->texSrvHeapIdx, pCore->cbvSrvUavDescSize);
        pCore->cmdList->SetGraphicsRootDescriptorTable(3, texHandle);

        Vsubmesh ritemMain = ppRitem[i]->mesh->objects["main"];
        pCore->cmdList->DrawIndexedInstanced(ritemMain.indexCount, 1, ritemMain.startIndexLocation, ritemMain.baseVertexLocation, 0);
    }
}

UINT calcConstBuffSize(UINT byteSize)
{
    // Constant buffers must be a multiple of the minimum hardware
    // allocation size (usually 256 bytes).  So round up to nearest
    // multiple of 256.  We do this by adding 255 and then masking off
    // the lower 2 bytes which store all bits < 256.
    // Example: Suppose byteSize = 300.
    // (300 + 255) & ~255
    // 555 & ~255
    // 0x022B & ~0x00ff
    // 0x022B & 0xff00
    // 0x0200
    // 512
    return (byteSize + 255) & ~255;
}

void createConstBuffPair(D3DCore* pCore, size_t elemSize, UINT elemCount,
    BYTE** ppBuffCPU, ID3D12Resource** ppBuffGPU)
{
    // Create GPU buffer & CPU mapped data block
    checkHR(pCore->device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(calcConstBuffSize(elemSize) * elemCount),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(ppBuffGPU)));
    checkHR((*ppBuffGPU)->Map(0, nullptr, reinterpret_cast<void**>(ppBuffCPU)));
}