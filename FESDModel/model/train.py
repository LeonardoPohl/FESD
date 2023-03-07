from utils import AvgMeter, clip_gradient
import torch

# training
def train(train_loader, model, optimizer, criterion, scheduler, clip, epoch, epochs):
    loss_record = AvgMeter()

    correct = 0
    for i, pack in enumerate(train_loader, start=1):
        optimizer.zero_grad()
        
        rgbs, depths, poses_2d, errors = pack
        
        # RD3D
        rgbs = rgbs.cuda()
        depths = depths.cuda()
        print(rgbs.shape)
        print(depths.shape)
        rgbs = rgbs.unsqueeze(2)
        depths = depths.unsqueeze(2)

        # probably not correct
        images = torch.cat([rgbs, depths], 2)
        
        # Seperate Network
        poses = poses_2d.cuda()

        gt = errors.cuda()
        gt = gt.squeeze()

        pred_s = model(images, poses)
        if (i == 1 and False):
            make_dot(pred_s.mean(), params=dict(list(model.named_parameters()))).render("rnn_torchviz", format="png")

        # TODO Calculate different loss based on the error label
        loss = criterion(pred_s, gt)
        
        loss.backward()
        clip_gradient(optimizer, clip)
        optimizer.step()
        scheduler.step()
        
        loss_record.update(loss.data, 1)
        correct += (pred_s == gt).float().sum()

        if i % 100 == 0 or i == len(train_loader):
          print(pred_s)
          print(gt)
          print('Epoch [{:03d}/{:03d}], Step [{:04d}/{:04d}], Loss: {:.4f}, Accuracy: {:.4f}'.format(epoch, epochs, i, len(train_loader),
                               loss_record.show(), correct / 100))
          correct = 0
    #     images = images.cuda()
    #     gts = gts.cuda()
    #     depths = depths.cuda()

    #     # multi-scale training samples
    #     trainsize = int(round(opt.trainsize * rate / 32) * 32)
    #     if rate != 1:
    #         images = F.upsample(images, size=(trainsize, trainsize), mode='bilinear', align_corners=True)
    #         images = images.unsqueeze(2)
    #         gts = F.upsample(gts, size=(trainsize, trainsize), mode='bilinear', align_corners=True)

    #         depths = F.upsample(depths, size=(trainsize, trainsize), mode='bilinear', align_corners=True)
    #         depths = depths.unsqueeze(2)
    #         images = torch.cat([images, depths], 2)

    #     if rate == 1:
    #         images = images.unsqueeze(2)
    #         depths = depths.unsqueeze(2)
    #         images = torch.cat([images, depths], 2)

    #     # forward
    #     pred_s = model(images)
    #     # TODO Calculate different loss based on the error label
    #     loss = criterion(pred_s, gts)

    #     loss.backward()
    #     clip_gradient(optimizer, opt.clip)
    #     optimizer.step()
    #     scheduler.step()
    #     if rate == 1:
            
    #         loss_record.update(loss.data, opt.batchsize)
      